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

```
# mkdir Build && cd Build
# cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DALLOW_DOWNLOADS=ON \
  -DUSE_SYSTEM_MONGOOSE=OFF \
  -DUSE_SYSTEM_PAHO=OFF \
  -DUSE_SYSTEM_GOOGLE_TEST=OFF
# make
```


Static Linking
--------------

Static linking against all third-party dependencies is also available
in order to provide maximum portability, notably for Microsoft Windows
and Apple OS X:

```
# mkdir Build && cd Build
# cmake .. -DCMAKE_BUILD_TYPE=Release -DSTATIC_BUILD=ON
# make
```
