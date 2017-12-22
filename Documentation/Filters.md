Available filters
=================

This page documents the filters that are current provided by the
Atom-IT server:

 * [CSVSink](#csvsink)
 * [CSVSource](#csvsource)
 * [Counter](#counter)
 * [FileLines](#filelines)
 * [HttpPost](#httppost)
 * [IMST](#imst)
 * [LoRaDecoder](#loradecoder)
 * [Lua](#lua)
 * [MQTTSink](#mqttsink)
 * [MQTTSource](#mqttsource)

NB: The actual construction of all these filters is carried by the
[`AtomIT::CreateFilter()`](../Application/FilterFactory.cpp) factory
function.


CSVSink
-------

This sink filter receives the content of a time series, and writes it
to [CSV file](https://en.wikipedia.org/wiki/Comma-separated_values) on
the disk. Here is a sample CSV file:

```
Time series,Timestamp,Metadata,Value
"world",0,application/octet-stream,"QMUcASYADQAB+KP4jq/x"
"world",1,application/octet-stream,"QMUcASYADgABkamAnXsI"
"world",2,application/octet-stream,"QMUcASYAAAABUqfkS7xs"
```

The first column contains the identifier of the [time
series](Concepts.md#time-series), the second is the timestamp, the
third is the metadata, and the fourth is the value. By default, the
value is [Base64-encoded](https://en.wikipedia.org/wiki/Base64) so as
to gracefully cope with binary values. Note that the file can be
prefixed with a header line that explicitly indicates the content of
the four columns.

**Mandatory parameters:**

 * `Input`: The identifier of the input time series.
 * `Path`: Path to the output CSV file.
 * `Type`: String value that must be set to "`CSVSink`".

**Optional parameters:**

 * `Append`: Boolean parameter specifying what to do if the output
   file already exists on the disk at the startup of the Atom-IT
   server. If `true`, the file is not overwritten (i.e. new entries
   are appended). If `false`, the file is overwritten. The default is
   `true`.
 * `Base64`: Boolean value indicating whether to encode the value
   using [Base-64 encoding](https://en.wikipedia.org/wiki/Base64) (default: `true`).
 * `Header`: Boolean value indicating whether to write the header line
   at the beginning of the file (default: `false`).
 * [`Name`](#common-parameters).
 * [`PopInput`](#common-parameters).
 * [`ReplayHistory`](#common-parameters).


CSVSource
---------

This source filter reads the content of a [CSV
file](https://en.wikipedia.org/wiki/Comma-separated_values) produced
by a [CSVSink](#csvsink), and publishes it to a time series.  It
assumes that the default parameters of the CSVSink filter were used
(`Base64` is `true` and `Header` is `false`).

**Mandatory parameters:**

 * `Output`: The identifier of the output time series.
 * `Path`: Path to the input CSV file.
 * `Type`: String value that must be set to "`CSVSource`".

**Optional parameters:**

 * [`MaxPendingMessages`](#common-parameters).
 * [`Name`](#common-parameters).


Counter
-------

Sample source filter that publishes messages whose value consists of
a sequence of increasing numbers. This filter is useful for testing.

**Mandatory parameters:**

 * `Output`: The identifier of the output time series.
 * `Type`: String value that must be set to "`Counter`".

**Optional parameters:**

 * `Delay`: Unsigned integer value specifying the number of milliseconds to
   wait between each message (default: `100` milliseconds).
 * `Increment`: Unsigned integer value specifying the increment (default: `1`).
 * `Start`: Integer value specifying where to start the counter (default: `0`).
 * `Stop`: Integer value specifying where to stop the counter (default: `100`).
 * [`Metadata`](#common-parameters).
 * [`Name`](#common-parameters).


FileLines
---------

This source filter reads the content of a text file, and publishes each
line as a message into some time series.

**Mandatory parameters:**

 * `Output`: The identifier of the output time series.
 * `Path`: Path to the input CSV file.
 * `Type`: String value that must be set to "`FileLines`".

**Optional parameters:**

 * [`MaxPendingMessages`](#common-parameters).
 * [`Metadata`](#common-parameters).
 * [`Name`](#common-parameters).


HttpPost
--------

This sink filter receives the content of a time series, and makes
a HTTP POST request to some Web service for each input message.

**Mandatory parameters:**

 * `Input`: The identifier of the input time series.
 * `Url`: Destination URL for the HTTP POST requests.
 * `Type`: String value that must be set to "`HttpPost`".

**Optional parameters:**

 * `Timeout`: Unsigned integer value specifying the HTTP timeout
   in seconds (default: `10` seconds).
 * `Username`: String value giving the username for HTTP Basic Authentication.
 * `Password`: String value giving the password for HTTP Basic Authentication.
 * [`Name`](#common-parameters).
 * [`PopInput`](#common-parameters).
 * [`ReplayHistory`](#common-parameters).


IMST
----

This source filter is used to read LoRa packets from a [IMST iC880a
concentrator](https://wireless-solutions.de/products/radiomodules/ic880a.html)
connected to a [Raspberry
Pi](https://en.wikipedia.org/wiki/Raspberry_Pi). Check out the
[associated
story](https://www.thethingsnetwork.org/labs/story/setting-up-a-mobile-gateway)
on The Things Network Labs. A full [configuration
sample](SampleIMST.md) is also available.

**Mandatory parameters:**

 * `Output`: The identifier of the output time series.
 * `Type`: String value that must be set to "`IMST`".

**Optional parameters:**

 * [`Metadata`](#common-parameters).
 * [`Name`](#common-parameters).


LoRaDecoder
------------

This filter can be used to decode LoRa packets using ABP network keys.
It takes the raw packets from the input time series (e.g. coming from
an [IMST filter](#imst)), decodes them, and forward the decoded
packets to another time series. Check out the [standalone LoRa
gateway sample](SampleIMST.md) for an usage example.

Note that metadata is automatically set to the hardware EUI identifier.

**Mandatory parameters:**

 * `nwkSKey`: The ABP network encryption key (check out your Arduino code).
 * `appSKey`: The ABP application encryption key (check out your Arduino code).
 * `Input`: The identifier of the input time series.
 * `Output`: The identifier of the output time series.
 * `Type`: String value that must be set to "`LoRaDecoder`".

**Optional parameters:**

 * [`Name`](#common-parameters).
 * [`ReplayHistory`](#common-parameters).
 * [`PopInput`](#common-parameters).


Lua
---

Check out the page dedicated to [Lua scripting](Lua.md).

**Mandatory parameters:**

 * `Input`: The identifier of the input time series.
 * `Path`: Path to the Lua script to be executed for each incoming message.
 * `Type`: String value that must be set to "`Lua`".

**Optional parameters:**

 * `Output`: The output time series. If not specified, the Lua
   script must manually specify it.
 * [`Name`](#common-parameters).
 * [`PopInput`](#common-parameters).
 * [`ReplayHistory`](#common-parameters).



MQTTSink
--------

This sink filter receives the content of a time series, and publishes
each of its messages to a [MQTT message
broker](https://en.wikipedia.org/wiki/MQTT). The metadata of the
message specifies the MQTT topic of the message.

**Mandatory parameters:**

 * `Input`: The identifier of the input time series.
 * `Type`: String value that must be set to "`MQTTSink`".

**Optional parameters:**

 * `Broker`: Structure defining the parameters of the MQTT broker (see below).
 * `ClientID`: String value identifying the MQTT client.
 * [`Name`](#common-parameters).
 * [`PopInput`](#common-parameters).
 * [`ReplayHistory`](#common-parameters).

**Broker parameters:** By default, the Atom-IT server uses a MQTT
broker running on `localhost` on TCP port `1883`. Here is the format
to specify other network parameters:

```
{
  "Server" : "eu.thethings.network",
  "Username" : "jodogne",
  "Password" : "ttn-account-v2.XXX",
  "Port" : 1883
}
```


MQTTSource
----------

This source filter subscribes to a [MQTT message
broker](https://en.wikipedia.org/wiki/MQTT), listens to a set of
topics, and publishes the received events to a time series. The
metadata is set to the topic associated to the event.

**Mandatory parameters:**

 * `Output`: The identifier of the output time series.
 * `Type`: String value that must be set to "`MQTTSource`".

**Optional parameters:**

 * `Broker`: Structure defining the parameters of the MQTT broker (see
   [MQTTSink](#mqttsink)).
 * `ClientID`: String value identifying the MQTT client.
 * [`Name`](#common-parameters).
 * `Topics`: List of strings (possibly with `+` wildcards) specifying
   the topics to listen to. Check out [The Things Network
   sample](SampleTheThingsNetwork.md) for an example.


Common parameters
-----------------

 * `Name`: String that gives a name to the filter. This name is useful
   to identify problems in the logs.
 * `MaxPendingMessages`: Unsigned integer value that tells to
   limit the number of messages that are published to the output
   time series. When the maximum number of messages is reached,
   the source filter waits for an item to be removed from the
   time series before publishing new messages.
 * `Metadata`: String value to be associated as the metadata of the
   messages produced by this filter.
 * `PopInput`: If `true`, the filter will remove the received messages
   from the input time series. If `false`, the filter does not modify
   the input time series. The default is `false`. If some time series 
   is connected as the input of several filters, this parameter should
   always be set to `false`.
 * `ReplayHistory`: If `true`, the filter will read back the entire
   time series on the startup of the Atom-IT server. If `false`, the
   filter will only process messages stored in the time series after
   the startup. The default is `false`.
