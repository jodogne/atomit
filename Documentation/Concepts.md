Concepts of the Atom-IT server
==============================

The Atom-IT server provides a simple framework to define IoT
workflow. It is entirely built upon three basic concepts:

 1. [Time series](#time-series)
 2. [Backends](#backends)
 3. [Filters](#filters)
 
These three concepts are described on this page.


Time series
-----------
 
A time series is a **time-ordered set** of messages:

<pre>
        +---+---+----+----+----+----+----+----+----+----+
   ==>  | 5 | 9 | 10 | 18 | 22 | 30 | 31 | 35 | 40 | 43 |  ==>
        +---+---+----+----+----+----+----+----+----+----+
</pre>

Each **message** is made of:

 * A **timestamp**, that is a signed 64-bit integer.
 * A **value**, that is a string of arbitrary length that can contain
   binary characters.
 * A **metadata**, that is a string providing information about the
   value (such as a
   [MIME type](https://en.wikipedia.org/wiki/Media_type) or a
   [EUI hardware identifier](https://en.wikipedia.org/wiki/MAC_address)).

Within a single time series, **timestamps must increase
monotonically**. Messages can only be appended to the end of the time
series.

The interpretation of the timestamp is **application-specific**. For
instance, if the timestamps are interpreted as the number of
nanoseconds since the
[Epoch](https://en.wikipedia.org/wiki/Unix_time), any nanosecond
between years 1678 and 2261 can be indexed (*). Most commonly,
timestamps will consist of a sequence of numbers that is automatically
incremented by the Atom-IT server as messages are appended.

Once inserted in a time series, the content of the messages cannot be
updated. It is only possible to delete messages, by providing a range
of timestamps to remove. This behavior could be summarized as "**CRD
operations**",
i.e. [CRUD](https://en.wikipedia.org/wiki/Create,_read,_update_and_delete)
without Update. The Atom-IT server will ensure proper **mutual
exclusion** if several threads are simultaneously accessing the same
time series, according to the
[readers/writers paradigm](https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem).
It is also possible for a reading thread to wait until a change is
made by some writer thread on a given time series.

The Atom-IT server can hold several time series at once. The stored
**time series are all independent from each other** (in particular,
the interpretation of the timestamps can be different for different
time series).


NB: From a developer perspective, the content of the time series are
accessed through the
[`AtomIT::ITimeSeriesAccessor`](../Framework/TimeSeries/ITimeSeriesAccessor.h)
C++ interface that are constructed by the
[`AtomIT::ITimeSeriesManager`](../Framework/TimeSeries/ITimeSeriesManager.h)
object that is unique throughout the Atom-IT server.

(*) Rough computation with
[GNU Octave](https://en.wikipedia.org/wiki/GNU_Octave): `1970 ±
(2**63-1)/(1000*1000*1000*60*60*24*365.2425)`, where `365.2425` is the
mean number of days in a year according to the
[Gregorian calendar](https://en.wikipedia.org/wiki/Year#In_international_calendars).


Backends
--------

The **backends** are the software components of the Atom-IT server
that are responsible for storing and accessing the time series. They
must notably implement
[database transactions](https://en.wikipedia.org/wiki/Database_transaction)
to ensure the consistency of data.

For the time being, backends are provided to store time series either
directly **in RAM or onto local
[SQLite databases](https://en.wikipedia.org/wiki/SQLite)**. In the
future, backends supporting [OpenTSDB](http://opentsdb.net/),
[MySQL](https://en.wikipedia.org/wiki/MySQL),
[PostgreSQL](https://en.wikipedia.org/wiki/PostgreSQL) or
[HBase](https://en.wikipedia.org/wiki/Apache_HBase) databases might be
developed in order to improve scalability and to transparently share
time series between Atom-IT servers.

Backends are also responsible for assigning quotas to their time
series in order to **recycle space** by removing oldest messages as
new messages are appended. These quotas are expressed as the maximum
number of messages in the time series, and as the maximum total size
of the values of the messages in the time series.

One single Atom-IT server can manage several backends at once, each of
them storing several time series in different places. This is
illustrated in the following figure:

<pre>
                                                       +---------------+
                                                   +-- | Time series 1 |
                           +-------------------+   |   +---------------+
                       +-- |  Memory backend   | --+
                       |   +-------------------+   |   +---------------+
  +----------------+   |                           +-- | Time series 2 |
  | Atom-IT server | --+                               +---------------+
  +----------------+   |
                       |                               +---------------+
                       |                           +-- | Time series 3 |
                       |   +-------------------+   |   +---------------+
                       +-- | SQLite "atom1.db" | --+
                       |   +-------------------+   |   +---------------+
                       |                           +-- | Time series 4 |
                       |                               +---------------+
                       |
                       |   +-------------------+       +---------------+
                       +-- | SQLite "atom2.db" | ----- | Time series 5 |
                           +-------------------+       +---------------+
</pre>



NB 1: From a developer perspective, the backends must implement the
[`AtomIT::ITimeSeriesBackend`](../Framework/TimeSeries/ITimeSeriesBackend.h)
C++ interface, and are constructed through
[`AtomIT::ITimeSeriesFactory`](../Framework/TimeSeries/ITimeSeriesFactory.h)
factories.

NB 2: In the currently available SQLite backend, the messages are
expected to be relatively small (say, up to about 1KB).  An improved
data storage scheme to store larger messages (e.g. images for video
streams) is work-in-progress.

 
Filters
-------

A **filter** is an object that processes input messages coming from
different time series, and that combines them to compute output
messages that are pushed into other time series:

<pre>
  +---------------------+
  | Input time series 1 | >--+                       +----------------------+
  +---------------------+    |                  +--> | Output time series 1 |
                             |                  |    +----------------------+
  +---------------------+    |    +--------+    |
  | Input time series 2 | >--+--> | Filter | >--+             ...
  +---------------------+    |    +--------+    |
                             |                  |    +----------------------+
            ...              |                  +--> | Output time series M |
                             |                       +----------------------+
  +---------------------+    |
  | Input time series N | >--+
  +---------------------+
</pre>

The number of inputs of a filter can be different to the number of its
outputs. Some special cases of filters commonly arise:

 * A **source** is a filter with no input time series.
 * A **sink** is a filter with no output time series. 
 * An **adapter** is a filter with exactly one input time series.
 
Sources and sinks will most often correspond to a file, a network
adapter, or a hardware device. As far as adapters are concerned, they
would convert the value of messages, and/or implement
[demultiplexing](https://en.wikipedia.org/wiki/Demultiplexer_(media_file))
to output time series.

Filters must implement a very simple **state machine**:

<pre>
                                 Step()    => In one thread for each filter
                                +------+
                               /        \
                              /          \
                              \          /
 +---------+     +---------+   \        /    +--------+     +------------+
 | Factory | --> | Start() | ---+------+---> | Stop() | --> | Destructor |
 +---------+     +---------+                 +--------+     +------------+
</pre>

As depicted above:

 1. A global factory function creates all the filter objects given the
    configuration file of the Atom-IT server.
 2. Once all the filters are created, the Atom-IT server invokes the
    `Start()` method of each of them. If some filter fails to start,
    the Atom-IT server does not start at all.
 3. Once all the filters are started, one separate thread is created
    for each of them. This thread will repeatedly invoke the `Step()`
    method of the filter in an infinite loop. The `Step()` method is
    assumed to have a bounded execution time (ideally, no more than
    500ms).
 4. Once the Atom-IT server is asked to stop (typically with a
    Control-C keypress or any
    [kill command](https://en.wikipedia.org/wiki/Kill_(command))), all
    the threads are stopped by canceling the infinite loop. Once each
    filter has stopped, the Atom-IT server invokes the `Stop()` method
    to give each filter a chance to cleanly release its allocated
    resources, and destruct each filter object.

NB: From a developer perspective, filters must implement the
[`AtomIT::IFilter`](../Framework/Filters/IFilter.h) C++ interface, and
are constructed through the
[`AtomIT::CreateFilter()`](../Application/FilterFactory.h) global
factory function.
