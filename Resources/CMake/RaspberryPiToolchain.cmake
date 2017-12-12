##
## CMake toolchain for Raspberry Pi, using Crosstool-NG
## http://crosstool-ng.github.io/
## https://www.bootc.net/archives/2012/05/26/how-to-build-a-cross-compiler-for-your-raspberry-pi/
##
## Usage:
## cmake -DCROSSTOOLS_ROOT=/home/jodogne/x-tools/ -DSTATIC_BUILD=ON -DCMAKE_TOOLCHAIN_FILE=../Resources/CMake/RaspberryPiToolchain.cmake -DCMAKE_BUILD_TYPE=Release .. 
##

if (NOT DEFINED CROSSTOOLS_ROOT)
  # If not given at the CMake invokation, use the default target
  # location of Crosstool-NG (i.e. "~/x-tools", where the user
  # directory "~" is retrieved by reading the USER environment
  # variable)
  set(CROSSTOOLS_ROOT "/home/$ENV{USER}/x-tools")
endif()

if (NOT DEFINED CROSSTOOLS_ARCHITECTURE)
  set(CROSSTOOLS_ARCHITECTURE arm-unknown-linux-gnueabi)
endif()
  
# the name of the target operating system
set(CMAKE_SYSTEM_NAME Linux)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER ${CROSSTOOLS_ROOT}/${CROSSTOOLS_ARCHITECTURE}/bin/${CROSSTOOLS_ARCHITECTURE}-gcc)
set(CMAKE_CXX_COMPILER ${CROSSTOOLS_ROOT}/${CROSSTOOLS_ARCHITECTURE}/bin/${CROSSTOOLS_ARCHITECTURE}-g++)

# here is the target environment located
set(CMAKE_FIND_ROOT_PATH ${CROSSTOOLS_PATH}/${CROSSTOOLS_ARCHITECTURE})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
