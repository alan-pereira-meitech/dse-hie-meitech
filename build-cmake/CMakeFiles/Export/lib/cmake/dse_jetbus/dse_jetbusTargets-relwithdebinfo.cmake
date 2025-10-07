#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dse::jetbus_core" for configuration "RelWithDebInfo"
set_property(TARGET dse::jetbus_core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(dse::jetbus_core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libjetbus_core.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS dse::jetbus_core )
list(APPEND _IMPORT_CHECK_FILES_FOR_dse::jetbus_core "${_IMPORT_PREFIX}/lib/libjetbus_core.a" )

# Import target "dse::dse_device" for configuration "RelWithDebInfo"
set_property(TARGET dse::dse_device APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(dse::dse_device PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libdse_device.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS dse::dse_device )
list(APPEND _IMPORT_CHECK_FILES_FOR_dse::dse_device "${_IMPORT_PREFIX}/lib/libdse_device.a" )

# Import target "dse::dse_console_example" for configuration "RelWithDebInfo"
set_property(TARGET dse::dse_console_example APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(dse::dse_console_example PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/dse_console_example"
  )

list(APPEND _IMPORT_CHECK_TARGETS dse::dse_console_example )
list(APPEND _IMPORT_CHECK_FILES_FOR_dse::dse_console_example "${_IMPORT_PREFIX}/bin/dse_console_example" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
