#----------------------------------------------------------------
# Generated CMake target import file for configuration "RELEASE".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mare" for configuration "RELEASE"
set_property(TARGET mare APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(mare PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libmare.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS mare )
list(APPEND _IMPORT_CHECK_FILES_FOR_mare "${_IMPORT_PREFIX}/lib/libmare.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
