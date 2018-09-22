#!/bin/bash

set -e
cd

# Create the various directories
mkdir /etc/atomit
mkdir -p /var/lib/atomit/db

# Create a basic configuration
cat << EOF > /etc/atomit/atomit.json
{
  "AutoTimeSeries" : { },  // Automatic use of the memory backend
  "HttpPort" : 8042,
  "RemoteAccessAllowed" : true,
  "AuthenticationEnabled" : true,
  "RegisteredUsers" : {
    "atomit" : "atomit"
  },
  "TimeSeries" : {
    // "hello" : {
    //  "Backend" : "SQLite",
    //  "Path" : "/var/lib/atomit/db/hello.db"
    //}
  }
}
EOF
