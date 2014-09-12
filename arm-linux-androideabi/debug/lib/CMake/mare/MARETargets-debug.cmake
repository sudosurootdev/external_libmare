#----------------------------------------------------------------
# Generated CMake target import file for configuration "DEBUG".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mare" for configuration "DEBUG"
set_property(TARGET mare APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(mare PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libmare.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS mare )
list(APPEND _IMPORT_CHECK_FILES_FOR_mare "${_IMPORT_PREFIX}/lib/libmare.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
