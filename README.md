![Atom-IT](Resources/AtomITLogoDocumentation.png "Atom-IT")

The **Atom-IT server** is a lightweight, cross-platform, RESTful
[microservice](https://en.wikipedia.org/wiki/Microservices) for
[Internet of Things](https://en.wikipedia.org/wiki/Internet_of_things)
(IoT) applications.

Features
--------

The Atom-IT server provides a simple framework to define IoT workflows
through configuration files. Its small footprint makes it usable on
most hardware architectures, from the Raspberry Pi to cloud
systems. Its simplicity also makes it a valuable candidate to showcase
IoT networks as a vendor-neutral pedagogical platform.

The software comes out-of-the-box with the following features:

 * Centralization of time series coming from multiple data sources
   and different network protocols.
 * Built-in Web server that can be used to receive messages from
   callbacks provided by professional IoT networks (such as Sigfox and
   Proximus MyThings).
 * [Lua scripting](https://en.wikipedia.org/wiki/Lua_(programming_language))
   to transcode packets.
 * Support of the [MQTT protocol](https://en.wikipedia.org/wiki/MQTT), both as
   publisher and subscriber, which can be used to receive messages
   from [The Things Network](https://www.thethingsnetwork.org/) broker.
 * Support of the
   [IMST iC880A board](https://wireless-solutions.de/products/radiomodules/ic880a.html)
   to setup standalone [LoRa](https://www.lora-alliance.org/) gateways
   with a [Raspberry Pi](https://www.raspberrypi.org/).
 * Decoding of LoRa packets.
 * HTTP POST client in order to transmit messages to other IoT
   systems.
 * Built-in [REST API](Documentation/RestApi.md) that can be used to
   drive the server from external heavyweight software, from scripts,
   or from Web applications.
 * Export and import to/from files (notably in the
   [CSV file format](https://en.wikipedia.org/wiki/Comma-separated_values)).
 * Long-term storage of messages within [SQLite](https://www.sqlite.org/) databases.
 
The server also features a simple Web interface in order to explore
the content of its database. 


Documentation
-------------

Documentation is work-in-progress, and will progressively be made
available in the following days.

 * [Quick Start](Documentation/QuickStart.md)
 * [Compilation](Documentation/Compilation.md)
 * [Basic concepts](Documentation/Concepts.md)
 * [Configuration](Documentation/Configuration.md)
 * [Available filters](Documentation/Filters.md)
 * [REST API](Documentation/RestApi.md)
 * [Lua scripting](Documentation/Lua.md)
 * Samples of IoT workflow:
   * [Connecting to Proximus MyThings](Documentation/SampleProximusMyThings.md)
   * [Connecting to Sigfox](Documentation/SampleSigfox.md)
   * [Connecting to The Things Network](Documentation/SampleTheThingsNetwork.md)
   * [Standalone LoRa gateway](Documentation/SampleIMST.md)

If you don't find an answer to some question in this documentation,
please ask the
[official discussion forum](https://groups.google.com/forum/#!forum/atomit-users)!


Licensing
---------

The Atom-IT server is provided as free and open-source software
licensed under GPLv3+ courtesy of [WSL](http://www.wsl.be/). WSL is
the key partner of techno-entrepreneurs in the Walloon region of
Belgium. The Atom-IT server was created as a shared platform to
support the [Atom-IT project](http://www.atomit.be/) by WSL.

