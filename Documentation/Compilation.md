Compilation
===========

The Atom-IT server is mainly written in C++. Its infrastructure is
based upon the [Orthanc framework](https://www.orthanc-server.com/)
for medical imaging. As a consequence, please refer to the
[build instructions for Orthanc](https://bitbucket.org/sjodogne/orthanc/src/default/LinuxCompilation.txt?fileviewer=file-view-default)
to get an insight about how to compile it for your platform.


Ubuntu 16.04 LTS
----------------

The following build instruction is known to work properly on Ubuntu
16.04:

```bash
$ mkdir Build && cd Build
$ cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DALLOW_DOWNLOADS=ON \
  -DUSE_SYSTEM_MONGOOSE=OFF \
  -DUSE_SYSTEM_PAHO=OFF \
  -DUSE_SYSTEM_GOOGLE_TEST=OFF
$ make
```


Static Linking
--------------

Static linking against all third-party dependencies is also available
in order to provide maximum portability, notably for Microsoft Windows
and Apple OS X:

```bash
$ mkdir Build && cd Build
$ cmake .. -DCMAKE_BUILD_TYPE=Release -DSTATIC_BUILD=ON
$ make
```


Docker
------

[Docker](https://en.wikipedia.org/wiki/Docker_(software)) images for the Atom-IT server are freely available on the [DockerHub](https://hub.docker.com/r/jodogne/atomit/) platform. The source code of the corresponding Docker images is available within [this repository](../Resources/Docker/), and are built on the top of the lightweight [Alphine distribution](https://hub.docker.com/_/alpine/).

### Starting Atom-IT

Here is the command to start the Atom-IT server using Docker:

```bash
$ sudo docker pull jodogne/atomit
$ sudo docker run -p 8042:8042 --rm jodogne/atomit
```

The Web interface and [REST API](RestApi.md) are then accessible on http://localhost:8042/. The default username is `atomit` and the default password is `atomit`. This Docker image is configured to [auto-create time series using the memory backend](Configuration.md#auto-creation-of-time-series).

Passing additional command-line options (e.g. to make Atom-IT verbose) can be done as follows (note the `/etc/atomit/atomit.json` option that is required for the server to find its configuration file):

```bash
$ sudo docker run -p 8042:8042 --rm jodogne/atomit /etc/atomit/atomit.json --verbose
```

### Fine-tuning the configuration

You can generate a custom configuration file for the Atom-IT server as follows. First, retrieve the default [configuration file](Configuration.md):

```bash
$ sudo docker run --rm --entrypoint=cat jodogne/atomit /etc/atomit/atomit.json > /tmp/atomit.json
```

Then, edit the just-generated file `/tmp/atomit.json` and restart Atom-IT with your updated configuration:

```bash
$ sudo docker run -p 8042:8042 --rm -v /tmp/atomit.json:/etc/atomit/atomit.json:ro jodogne/atomit
```

### Making storage persistent

The filesystem of Docker containers is volatile (its content is deleted once the container stops). You can make the Atom-IT database persistent by defining a SQLite database in the `/var/lib/atomit/db` folder of the container, and mapping this folder somewhere in the filesystem of your Linux host, e.g.:

```bash
$ cat << EOF > /tmp/atomit.json
{
  "AutoTimeSeries" : {
    "Backend" : "SQLite",
    "Path" : "/var/lib/atomit/db/atomit.db"
  },
  "HttpPort" : 8042,
  "RemoteAccessAllowed" : true,
  "AuthenticationEnabled" : true,
  "RegisteredUsers" : {
    "atomit" : "atomit"
  }
}
EOF
$ sudo docker run -p 8042:8042 --rm \
                  -v /tmp/atomit.json:/etc/atomit/atomit.json:ro \
                  -v /tmp/atomit-db/:/var/lib/atomit/db/ \
                  jodogne/atomit /etc/atomit/atomit.json --verbose
```
