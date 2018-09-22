#/bin/sh

set -e

# Get the number of available cores to speed up the build
COUNT_CORES=`grep -c ^processor /proc/cpuinfo`
echo "Will use $COUNT_CORES parallel jobs to build the Atom-IT server"

# Create the various directories as in official Debian packages
mkdir /etc/atomit
mkdir -p /var/lib/atomit/db

# Clone the Atom-IT repository and switch to the requested branch
cd /root/
git clone https://github.com/jodogne/atomit.git atomit
cd atomit
echo "Switching to branch: $1"
git checkout "$1"

# Build the Atom-IT server
mkdir Build
cd Build
cmake \
    -DALLOW_DOWNLOADS=ON \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DSTATIC_BUILD=ON \
    -DENABLE_IMST_GATEWAY=OFF \
    -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations" \
    ..
make -j$COUNT_CORES

# Run the unit tests
./UnitTests

# Install the Atom-IT server
make install

# Remove the build directory to recover space
cd /root/
rm -rf /root/atomit

# Generate a minimal configuration file
CONFIG=/etc/atomit/atomit.json

cat <<EOF > $CONFIG
{
  "AutoTimeSeries" : { },  // Automatic use of the memory backend
  "HttpPort" : 8042,
  "RemoteAccessAllowed" : true,
  "AuthenticationEnabled" : true,
  "RegisteredUsers" : {
    "atomit" : "atomit"
  }
}
EOF
