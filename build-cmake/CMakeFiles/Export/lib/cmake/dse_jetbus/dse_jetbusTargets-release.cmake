#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dse::jetbus_core" for configuration "Release"
set_property(TARGET dse::jetbus_core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(dse::jetbus_core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libjetbus_core.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS dse::jetbus_core )
list(APPEND _IMPORT_CHECK_FILES_FOR_dse::jetbus_core "${_IMPORT_PREFIX}/lib/libjetbus_core.a" )

# Import target "dse::dse_device" for configuration "Release"
set_property(TARGET dse::dse_device APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(dse::dse_device PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libdse_device.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS dse::dse_device )
list(APPEND _IMPORT_CHECK_FILES_FOR_dse::dse_device "${_IMPORT_PREFIX}/lib/libdse_device.a" )

# Import target "dse::dse_stream_1s" for configuration "Release"
set_property(TARGET dse::dse_stream_1s APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(dse::dse_stream_1s PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/dse_stream_1s"
  )

list(APPEND _IMPORT_CHECK_TARGETS dse::dse_stream_1s )
list(APPEND _IMPORT_CHECK_FILES_FOR_dse::dse_stream_1s "${_IMPORT_PREFIX}/bin/dse_stream_1s" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
