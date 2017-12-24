Configuration
=============

Before reading this section, make sure to understand the
[concepts](Concepts.md) of the Atom-IT server.

The Atom-IT server instantiates its time series, backends, filters,
and embedded [Web server](RestApi.md) by reading a **configuration
file**. The configuration file is written according to the
[JSON file format](https://en.wikipedia.org/wiki/JSON).  Once the
configuration is written to a file (say, `Configuration.json`), the
Atom-IT can be started as follows:

```bash
$ ./AtomIT --verbose Configuration.json
```

The `--verbose` argument provides debugging information, and can be
safely removed in production environments. You can find all the
available command-line arguments of the Atom-IT server by adding the
`--help` argument:

```bash
$ ./AtomIT --help
```

This section reviews the 3 parts
of the configuration file, namely:

 1. [Time series definition](#time-series-definition),
 2. [Filters construction](#filters-construction),
 3. [Web server parameters](#web-server-parameters).
 
 
Time series definition
----------------------

All the time series used by the filters running within the Atom-IT
server must be defined within a member of the configuration file that
is called `TimeSeries`. This member is an object that provides the
configuration of each time series.


### Memory backend

The most basic time series definition is the following:

```javascript
{
  "TimeSeries" : {
    "hello" : {}
  }
}
```

This definition will create a time series identified with the name
`hello` using the **default memory backend**. The use of the memory
backend can be made explicit as follows:

```javascript
{
  "TimeSeries" : {
    "hello" : {
      "Backend" : "Memory"
    }
  }
}
```

Multiple time series can of course be created within the Atom-IT server:

```javascript
{
  "TimeSeries" : {
    "hello" : { },
    "world" : { }
  }
}
```


### Timestamps policy

Each time series can be assigned with a timestamp assignment policy.
This policy tells the Atom-IT server how to assign the timestamps to
messages appended without an explicit timestamp:

```javascript
{
  "TimeSeries" : {
    "hello" : {
      "Backend" : "Memory",
      "Timestamp" : "NanosecondsClock"
    }
  }
}
```

In this case, messages will be tagged with the system time at which
they were appended to the time series, expressed in nanoseconds. The
possible values are as follows:

 * `Sequence`: Automatically increasing sequence number (the default).
 * `NanosecondsClock`: Number of nanoseconds since the
   [Epoch](https://en.wikipedia.org/wiki/Unix_time).
 * `MillisecondsClock`: Number of milliseconds since the
   [Epoch](https://en.wikipedia.org/wiki/Unix_time).
 * `SecondsClock`: Number of seconds since the
   [Epoch](https://en.wikipedia.org/wiki/Unix_time).

**Warning**: As the timestamps must be
  [monotonically increasing](Concepts.md#time-series) within a time
  series, messages with identical timestamps will be dropped.


### Quotas

It is possible to **assign a quota** separately to each time series:

```javascript
{
  "TimeSeries" : {
    "hello" : {
      "Backend" : "Memory",
      "MaxSize" : 1024,   // 1KB
      "MaxLength" : 10
    }
  }
}
```

This quota specifies the maximum number of messages that can be stored
by the time series (`MaxLength`), as well as a maximum for the
total size of all the values stored in the time series (`MaxSize`
expressed in bytes). If the quota is exceeded because of the appending
of one new message to the time series, the oldest messages are removed
until the quota is met again. `MaxLength` and `MaxSize` are both
optional, and can be different for each time series.



### SQLite backend

It is possible to ask the Atom-IT server to store time series on the
filesystem as a [SQLite database](https://www.sqlite.org/). This
enables persistence of time series across restarts of the Atom-IT
server, and increases the available storage by storing data on the
disk instead of in RAM. You just need to specify the path to the
SQLite database:

```javascript
{
  "TimeSeries" : {
    "hello" : {
      "Backend" : "SQLite",
      "Path" : "iot.db",
      "MaxSize" : 1048576,   // 1MB
      "MaxLength" : 1000
    }
  }
}
```

Note that quotas are also available for the SQLite backend, thanks to
the `MaxSize` and `MaxLength` optional arguments.

The Atom-IT server can freely be configured to store several time
series into the same SQLite database, or to store different time
series into different SQLite databases. Furthermore, different
backends can be used simultaneously. For instance, the
[sample topology](Concepts.md#backends) of the page describing the
basic concepts could be defined using the following configuration
file:

```javascript
{
  "TimeSeries" : {
    "time-series-1" : { },  // Default memory backend
    "time-series-2" : { },  // Default memory backend
    "time-series-3" : {
      "Backend" : "SQLite",
      "Path" : "atom1.db"
    },
    "time-series-4" : {
      "Backend" : "SQLite",
      "Path" : "atom1.db"
    },
    "time-series-5" : {
      "Backend" : "SQLite",
      "Path" : "atom2.db"
    }
  }
}
```

**Warning**: Never try and read a SQLite database while the Atom-IT
  server is running, otherwise this could result in data corruption.


### Auto-creation of time series

If the time series are not known before starting the Atom-IT server,
you can specify a default backend that will be used to create new
time series on-the-fly as they are needed by the filters. This
is done with the `AutoTimeSeries` configuration option.

The following sample configuration would for instance automatically
create new time series using the default memory backend, applying a
quota of maximum 100 messages, and using the nanoseconds-resolution
clock:

```javascript
{
  "AutoTimeSeries" : {
    "Backend" : "Memory",
    "MaxLength" : 100,
    "Timestamp" : "NanosecondsClock"
  }
}
```

Obviously, you can use a SQLite database as well:

```javascript
{
  "AutoTimeSeries" : {
    "Backend" : "SQLite",
    "Path" : "auto.db"
  }
}
```


**Remark**: If no configuration file is provided while starting the
Atom-IT server, the effect is the same as using an auto-creation of
time series by the memory backend.


Filters construction
--------------------

The construction of the filters consists in listing them to a
`Filters` member, as depicted in the following sample configuration
file:

```javascript
{
  "AutoTimeSeries" : { },
  "Filters" : [
    {
      "Name" : "counter",
      "Type" : "Counter",
      "Output" : "hello"
    }
  ]
}
```

This sample configuration will start the Atom-IT series with a single
filter named `counter`. The filter is of type `Counter`, that is a
basic demo [source filter](Concepts.md#filters) generating one message
each 100ms, whose value increases from 0 to 100. This message is
appended to the time series `hello`, that is auto-created.

The available filters together with their parameters are listed on a
[separated page](Filters.md) of the documentation.

You are obviously free to add multiple filters to define a workflow.
Several examples of more complex workflow are available at
[another place](Samples.md) of the documentation.


Web server parameters
---------------------

The Atom-IT server comes bundled with an embedded Web server. This Web
server is used to [serve the REST API](RestApi.md) and to provide a
minimalistic Web interface to browse the content of the time series.

By default, the embedded Web server is started and can be accessed on
http://localhost:8042/, but only from the localhost. This default
behavior can be fine-tuned with the following set of parameters in the
JSON configuration file:

```javascript
{
  "HttpServerEnabled" : true,       // Disable the HTTP server
  "HttpPort" : 8042,                // Set the HTTP port number
  "RemoteAccessAllowed" : false,    // Allow access from other computers than localhost
  "AuthenticationEnabled" : false,  // Enable HTTP Basic Authentication
  "RegisteredUsers" : {             // List of the registered users with passwords
    // "alice" : "alicePassword"
  }
}
```
