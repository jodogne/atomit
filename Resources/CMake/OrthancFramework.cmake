include(${CMAKE_CURRENT_LIST_DIR}/DownloadPackage.cmake)
set(ORTHANC_SOURCES_DIR ${CMAKE_BINARY_DIR}/Orthanc-1.3.1)

if (IS_DIRECTORY "${ORTHANC_SOURCES_DIR}")
  set(FirstRun OFF)
else()
  set(FirstRun ON)
endif()

DownloadPackage(
  "dac95bd6cf86fb19deaf4e612961f378"
  "https://www.orthanc-server.com/downloads/get.php?path=/orthanc/Orthanc-1.3.1.tar.gz"
  "${ORTHANC_SOURCES_DIR}")

if (FirstRun)
  execute_process(
    COMMAND ${PATCH_EXECUTABLE} -p0 -N -i
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/CMake/Orthanc-1.3.1.patch
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    RESULT_VARIABLE Failure
    )

  if (Failure)
    message(FATAL_ERROR "Error while patching a file")
  endif()
endif()

include(${ORTHANC_SOURCES_DIR}/Resources/CMake/OrthancFrameworkParameters.cmake)
set(ENABLE_GOOGLE_TEST ON)
set(ENABLE_LUA ON)
set(ENABLE_PUGIXML ON)
set(ENABLE_SQLITE ON)
set(ENABLE_WEB_CLIENT ON)
set(ENABLE_WEB_SERVER ON)
set(HAS_EMBEDDED_RESOURCES ON)
include(${ORTHANC_SOURCES_DIR}/Resources/CMake/OrthancFrameworkConfiguration.cmake)
