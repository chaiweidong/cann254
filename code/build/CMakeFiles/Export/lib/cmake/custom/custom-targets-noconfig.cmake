#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "custom::custom" for configuration ""
set_property(TARGET custom::custom APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(custom::custom PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libcust_opapi.so"
  IMPORTED_SONAME_NOCONFIG "libcust_opapi.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS custom::custom )
list(APPEND _IMPORT_CHECK_FILES_FOR_custom::custom "${_IMPORT_PREFIX}/lib/libcust_opapi.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
