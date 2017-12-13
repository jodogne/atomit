Atom-IT server
==============

The Atom-IT server is a lightweight, cross-platform, RESTful
[microservice](https://en.wikipedia.org/wiki/Microservices) for
[Internet of Things](https://en.wikipedia.org/wiki/Internet_of_things)
(IoT) applications.

The Atom-IT server is a simple framework to define IoT workflows
through configuration files. Its small footprint makes it usable on
many hardware architectures, from the Raspberry Pi to cloud systems.

The software comes out-of-the-box with the following features:

 * Built-in Web server that can be used to receive message from
   callbacks provided by professional IoT networks (such as Sigfox and
   Proximus MyThings).
 * [Lua scripting](https://en.wikipedia.org/wiki/Lua_(programming_language))
   to transcode packets.
 * Support of the [MQTT protocol](https://en.wikipedia.org/wiki/MQTT), both as
   publisher and subscriber, which can be used to receive messages
   from [The Things Network](https://www.thethingsnetwork.org/).
 * Support of the
   [IMST iC880A board](https://wireless-solutions.de/products/radiomodules/ic880a.html)
   to setup standalone [LoRa](https://www.lora-alliance.org/) gateways
   with a [Raspberry Pi](https://www.raspberrypi.org/).
 * Decryption of LoRa packets.
 * Support of HTTP POST client in order to transmit messages to other
   IoT systems.
 * Built-in REST API that can be used to drive the server from
   external applications or scripts.
 * Export and import to/from files (notably in the
   [CSV file format](https://en.wikipedia.org/wiki/Comma-separated_values)).
 * Long-term storage of messages within [SQLite](https://www.sqlite.org/) databases.
 
The server also features a simple Web interface in order to explore
the content of its database. 


Documentation
-------------

Documentation is work-in-progress, and will be available in the
following days.

 * [Basic concepts](Documentation/Concepts.md)


Sample
------

Here is a sample configuration file illustrating how to create a
simple IoT workflow collecting its data from The Things Network using
MQTT, decoding the payload, and mapping it as a REST API onto URL
`http://localhost:8042/series/decoded/`:

```
{
  "AutoTimeSeries" : {},
  "Filters" : [
    {
      "Type" : "MQTTSource",
      "Output" : "source",
      "Broker" : {
        "Server" : "eu.thethings.network",
        "Username" : "XXX",
        "Password" : "ttn-account-v2.XXX"
      },
      "Topics" : [
        "+/devices/+/up"
      ]
    },
    {
      "Type" : "Lua",
      "Path" : "ParseTheThingsNetwork.lua",
      "Input" : "source",
      "Output" : "decoded"
    }
  ]
}
```

The `ParseTheThingsNetwork.lua` Lua script implements the decoding of
the payload as follows:

```
function Convert(timestamp, metadata, rawValue)
  local value = ParseJson(rawValue)
  local device = value['hardware_serial']
  local payload = DecodeBase64(value['payload_raw'])
  
  local result = {}
  result['metadata'] = device
  result['value'] = payload

  return result
end
```

More samples will come in the following days.


Compilation
-----------

The Atom-IT server is mainly written in C++. Its infrastructure is
based upon the [Orthanc framework](https://www.orthanc-server.com/)
for medical imaging. As a consequence, please refer to the
[build instructions for Orthanc](https://bitbucket.org/sjodogne/orthanc/src/default/LinuxCompilation.txt?fileviewer=file-view-default)
to get an insight about how to compile it for your platform.

The following build instruction is known to work properly on Ubuntu
16.04:

```
# mkdir Build && cd Build
# cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DALLOW_DOWNLOADS=ON \
  -DUSE_SYSTEM_MONGOOSE=OFF \
  -DUSE_SYSTEM_PAHO=OFF \
  -DUSE_SYSTEM_GOOGLE_TEST=OFF
# make
```

Static linking against third-party dependencies is also available in
order to provide maximum portability, including on Microsoft Windows
and Apple OS X:

```
# mkdir Build && cd Build
# cmake .. -DCMAKE_BUILD_TYPE=Release -DSTATIC_BUILD=ON
# make
```


Licensing
---------

The Atom-IT server is provided as free and open-source software
licensed under GPLv3+ courtesy of [WSL](http://www.wsl.be/). WSL is
the key partner of techno-entrepreneurs in the Walloon region of
Belgium. The Atom-IT server was created as a shared platform to
support the [Atom-IT project](http://www.atomit.be/) by WSL.

