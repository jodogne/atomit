Concepts of the Atom-IT server
==============================

The Atom-IT server provides a simple framework to define IoT
workflows. It is entirely built upon three basic concepts:

 1. [Time series](#time-series)
 2. [Backends](#backends)
 3. [Filters](#filters)
 
These three concepts are described on this page.


Time series
-----------
 
A time series is an **time-ordered set** of messages:

<pre>
        +---+---+----+----+----+----+----+----+----+----+
   ==>  | 5 | 9 | 10 | 18 | 22 | 30 | 31 | 35 | 40 | 43 |  ==>
        +---+---+----+----+----+----+----+----+----+----+
</pre>

Each **message** is made of:

 * A **timestamp**, that is a signed 64-bit integer.
 * A **value**, that is a binary string.
 * A **metadata**, that is a string providing information about the
   value (such as a
   [MIME type](https://en.wikipedia.org/wiki/Media_type) or a
   [EUI hardware identifier](https://en.wikipedia.org/wiki/MAC_address)).

Within a single time series, **timestamps must increase
monotonically**. In other words, messages can only be appended to the
end of the time series.

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
exclusion** if several threads are accessing the time series,
according to the
[readers/writers paradigm](https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem).
It is also possible for a reading thread to wait until a change is
made by some writer thread on a given time series.

The Atom-IT server can hold several of such time series at once. The
stored **time series are all independent from each other** (in
particular, the interpretation of the timestamps can be different for
different time series).


NB: From a developer perspective, the content of the time series are
accessed through the
[`AtomIT::ITimeSeriesAccessor`](../Framework/TimeSeries/ITimeSeriesAccessor.h)
C++ interface that are constructed by the
[`AtomIT::ITimeSeriesManager`](../Framework/TimeSeries/ITimeSeriesManager.h)
[singleton object](https://en.wikipedia.org/wiki/Singleton_pattern).

(*) Rough computation with
[GNU Octave](https://en.wikipedia.org/wiki/GNU_Octave): `1970 ±
(2**63-1)/(1000*1000*1000*60*60*24*365.2425)`, where `365.2425` is the
mean number of days in a year according to the
[Gregorian calendar](https://en.wikipedia.org/wiki/Year#In_international_calendars).


Backends
--------

The **backends** are the software components of the Atom-IT server
that are responsible for storing the time series.

For the time being, backends are provided to store time series either
in **RAM or onto local
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
number of messages in the time series, and as the maxmimum total size
of the values of the messages in the time series.

One single Atom-IT server can manage several backends at once, each of
them storing several time series. This is illustrated in the following
figure:

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

NB 2: In the currently available backends (RAM and SQLite), the
messages are expected to be relatively small (say, up to about 1KB).
An improved data storage scheme to store larger messages (e.g. images
for video streams) is work-in-progress.

 
Filters
-------

WIP
