Lua scripting
=============

This section gives explanations about the behavior of the
[Lua filter](Filters.md#lua). This filter can be used to apply a
[Lua script](https://en.wikipedia.org/wiki/Lua_(programming_language))
to each message coming from a given time series. This script can:

  * Modify the timestamp, the metadata and/or the value of the
    incoming message.
  * Feed the result of the modification to a set of time series.  If
    the output is done to multiple time series, we will talk about a
    demultiplexer filter.
    
The Lua filter is configured by giving the `Path` parameter that
points to the file that contains the Lua script. If several Lua
filters are instanciated in the
[configurated workflow](Configuration.md), they can load a different
Lua script, and they run in full insolation from each other. This
notably implies that they cannot share variables.


Anatomy of the Lua script
-------------------------

### Function signature

The Lua callback must be called `Convert()` and must have the
following signature:

```lua
function Convert(timestamp, metadata, value)
  -- Callback
end
```

This script is invoked by the Atom-IT server as a callback whenever
the input time series (identified by the `Input` parameter of the
filter) receives a new message. The timestamp, metadata and value 
of the message are readily available for processing.


### Return value

The Lua script must return the modified message. A modified message is
specified as follows:

```lua
function Convert(timestamp, metadata, value)
  -- Modify "timestamp", "metadata", and "value"
  -- Possibly choose the "targetSeries"
  local result = {}
  result["timestamp"] = timestamp
  result["metadata"] = metadata
  result["value"] = value
  result["series"] = targetSeries
  return result
end
```

The `series` field specifies the target time series.

All of the fields of the `result` are optional. If `timestamp`,
`metadata` and/or `value` are absent, they are copied from the input
message. If `series` is absent, it is copied from the optional
`Output` parameter of the [Lua filter](Filters.md#lua).

The Lua script is allowed to return `nil` if the message must be
skipped.


### Demultiplexing

The Lua script is allowed to return a Lua table specifying a list of
messages. This allows to implement demultiplexing to multiple output
time series. Here is an example to demultiplex to 2 time series:

```lua
function Convert(timestamp, metadata, value)
  -- Callback
  local result = {}

  -- Provide the first modified message
  result[1] = {}
  result[1]["timestamp"] = timestamp1
  result[1]["metadata"] = metadata1
  result[1]["value"] = value1
  result[1]["series"] = targetSeries1

  -- Provide the second modified message
  result[2] = {}
  result[2]["value"] = value2
  result[2]["series"] = targetSeries2

  return result
end
```

If some of the fields (`value`, `metadata`, `value` or `series`) are
missing, they are completed [as described above](#return-value) in the
case of one output message.

As an illustration, here is the most basic Lua demultiplex, that
simply copies the input message to two different time series:

```lua
function Convert(timestamp, metadata, value)
  local result = {}
  result[1] = {}
  result[1]["series"] = "series1"
  result[2] = {}
  result[2]["series"] = "series2"
  return result
end
```


Examples
--------

Some examples of Lua scripts are available in the following sample
configurations:

 * [Connecting to Proximus MyThings](SampleProximusMyThings.md#parsing-the-proximus-mythings-payload)
 * [Connecting to Sigfox](SampleSigfox.md#parsing-the-sigfox-payload)
 * [Connecting to The Things Network](SampleTheThingsNetwork.md#decoding-the-payload)


Available functions
-------------------

The Lua scripts have full access to the Lua language and its
associated
[standard libraries](https://www.lua.org/manual/5.3/manual.html#6).

Besides the standard libraries, the following general-purpose
functions come from the Lua scripting engine of the
[Orthanc core](http://book.orthanc-server.com/users/lua.html#general-purpose-functions),
upon which the Atom-IT server is based:

 * `print(v)` prints the given value `v` to the Atom-IT log.
 * `ParseJson(s)` converts a string encoded in the JSON format to a Lua table.
 * `DumpJson(v, keepStrings)` encodes a Lua table as a JSON
   string. Setting the optional argument `keepStrings` to `true`
   prevents the automatic conversion of strings to integers.
 * `HttpGet(url, headers)` does a HTTP GET request to the given
   URL. It is possible to give HTTP headers as an associative array
   mapping strings to strings.
 * `HttpPost(url, body, headers)` does a HTTP POST request to the given URL,
   with the given HTTP `body` and HTTP `headers`.
 * `HttpPut(url, body, headers)` does a HTTP PUT request to the given URL,
   with the given HTTP `body` and HTTP `headers`.
 * `HttpDelete(url, headers)` does a HTTP DELETE request to the given URL,
   with the given HTTP `headers`.
 * `SetHttpCredentials(username, password)` sets the credentials for HTTP 
   Basic Authentication in subsequent HTTP requests.

In order to print the content of a complex Lua object, which can be
useful for debugging, you can copy-paste the `PrintRecursive(s)`
function that is part of the
[native Lua toolbox for Orthanc](https://bitbucket.org/sjodogne/orthanc/src/Orthanc-1.3.1/Resources/Toolbox.lua).

The Atom-IT server also adds the following functions that are useful
for IoT applications:

 * `ParseXml(s)` converts a string encoded in the XML format to a Lua table.
 * `DecodeBase64(s)` decodes a Base64-encoded string as a string
   (possibly containing binary characters).
 * `EncodeBase64(s)` encodes a binary string using the Base64 encoding.
 * `FormatHexadecimal(s)` encodes a binary string using its
   hexadecimal representation.
 * `ParseHexadecimal(s)` parses an hexadecimal-encoded binary string.
