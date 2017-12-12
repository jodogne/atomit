if (ENABLE_IMST_GATEWAY)

  # WARNING: Release 3.1.0 does not work with RPi! Necessary to use the mainline version
  
  set(IMST_GATEWAY_SOURCES_DIR ${CMAKE_BINARY_DIR}/lora_gateway-2017-09-13)
  DownloadPackage(
    "0548d61d760bceec4a6380e6c093b5bd"
    "http://www.orthanc-server.com/downloads/third-party/atom-it/lora_gateway-2017-09-13.tar.gz"
    "${IMST_GATEWAY_SOURCES_DIR}")

  file(WRITE
    ${IMST_GATEWAY_SOURCES_DIR}/libloragw/inc/config.h
    "
#pragma once
#define LIBLORAGW_VERSION  \"4.0.0\"
#define CFG_SPI_NATIVE  1
#define DEBUG_AUX       0
#define DEBUG_SPI       0
#define DEBUG_REG       0
#define DEBUG_HAL       0
#define DEBUG_GPS       0
#define DEBUG_GPIO      
#define DEBUG_LBT       0
#include \"imst_rpi.h\"
")

  include_directories(
    ${IMST_GATEWAY_SOURCES_DIR}/libloragw/inc/
    )
  
  set(IMST_GATEWAY_SOURCES
    ${IMST_GATEWAY_SOURCES_DIR}/libloragw/src/loragw_aux.c
    ${IMST_GATEWAY_SOURCES_DIR}/libloragw/src/loragw_fpga.c
    ${IMST_GATEWAY_SOURCES_DIR}/libloragw/src/loragw_gps.c
    ${IMST_GATEWAY_SOURCES_DIR}/libloragw/src/loragw_hal.c
    ${IMST_GATEWAY_SOURCES_DIR}/libloragw/src/loragw_lbt.c
    ${IMST_GATEWAY_SOURCES_DIR}/libloragw/src/loragw_radio.c
    ${IMST_GATEWAY_SOURCES_DIR}/libloragw/src/loragw_reg.c
    ${IMST_GATEWAY_SOURCES_DIR}/libloragw/src/loragw_spi.native.c
    )

  source_group(ThirdParty\\lora_gateway REGULAR_EXPRESSION ${IMST_GATEWAY_SOURCES_DIR}/.*)

  add_definitions(-DATOMIT_ENABLE_IMST_GATEWAY=1)

else()
  add_definitions(-DATOMIT_ENABLE_IMST_GATEWAY=0)

endif()
