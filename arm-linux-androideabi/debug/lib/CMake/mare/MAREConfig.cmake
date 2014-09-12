# - Config file for the MARE package
# It defines the following variables
#  MARE_INCLUDE_DIRS   - include directories for MARE
#  MARE_LIBRARIES      - libraries to link against
#  MARE_CXX_FLAGS      - compiler switches required for using MARE
#  MARE_HAVE_GPU       - GPU support is available
#  MARE_VERSION_STRING - the version of MARE found

# Compute paths
get_filename_component(MARE_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(MARE_INCLUDE_DIRS ${MARE_CMAKE_DIR}/../../../include)

# Our library dependencies (contains definitions for IMPORTED targets)
if(NOT TARGET mare AND NOT MARE_BINARY_DIR)
  include("${MARE_CMAKE_DIR}/MARETargets.cmake")
endif()
 
# These are IMPORTED targets created by MARETargets.cmake
set(MARE_LIBS mare)
# These are libraries that mare depends on
set(MARE_LIBS ${MARE_LIBS} )
find_package(Threads QUIET)
if(Threads_FOUND)
  set(MARE_LIBS ${MARE_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif(Threads_FOUND)

# and the -llog for android
if("True")
  set(MARE_LIBS ${MARE_LIBS} log)
endif()

# externally visible variables
list(APPEND MARE_INCLUDE_DIRS )
set(MARE_LIBRARIES ${MARE_LIBS})
set(MARE_CXX_FLAGS "-Wno-psabi --sysroot=/bard/wilma/manticore/Android/android-tools/android-ndk-r9d-google/platforms/android-8/arch-arm -fpic -funwind-tables -funswitch-loops -finline-limit=300 -fsigned-char -no-canonical-prefixes -march=armv7-a -mfloat-abi=softfp -mfpu=neon -fdata-sections -ffunction-sections -Wa,--noexecstack  -std=c++11  -pipe -W -Wall -Werror=return-type -Wall -Wextra -Weffc++ -Woverloaded-virtual -Wold-style-cast -Wshadow -Wnon-virtual-dtor -Wmissing-include-dirs -Wswitch-enum -Wno-lambda-extensions -frtti -fexceptions -DMARE_ENABLE_CHECK_API_DETAILED -O0 -g")
set(MARE_VERSION_STRING "0.11.0")
if(0)
  set(MARE_HAVE_GPU 1)
  set(MARE_CXX_FLAGS "${MARE_CXX_FLAGS} -DHAVE_OPENCL")
endif()

# flags on MSVC are selected at build time, since the VS project contain
# all configurations. So we directly add them to the build flags and do not
# define the generic MARE_CXX_FLAGS.
if(MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE         "${CMAKE_CXX_FLAGS_RELEASE}  -std=c++11")
  set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} -Wno-psabi --sysroot=/bard/wilma/manticore/Android/android-tools/android-ndk-r9d-google/platforms/android-8/arch-arm -fpic -funwind-tables -funswitch-loops -finline-limit=300 -fsigned-char -no-canonical-prefixes -march=armv7-a -mfloat-abi=softfp -mfpu=neon -fdata-sections -ffunction-sections -Wa,--noexecstack  -std=c++11  -pipe -W -Wall -Werror=return-type -Wall -Wextra -Weffc++ -Woverloaded-virtual -Wold-style-cast -Wshadow -Wnon-virtual-dtor -Wmissing-include-dirs -Wswitch-enum -Wno-lambda-extensions -frtti -fexceptions -DMARE_ENABLE_CHECK_API_DETAILED")
  set(CMAKE_CXX_FLAGS_MINSIZEREL      "${CMAKE_CXX_FLAGS_MINSIZEREL} ")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ")
endif(MSVC)

# Check that the compiler used for configuring the application is compatible
# with the compiler that was used to build MARE
if(MSVC)
  set(__mare_cxx_compiler_version ${MSVC_VERSION})
else()
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} --version
      OUTPUT_VARIABLE __mare_cxx_compiler_version)
  else()
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
      OUTPUT_VARIABLE __mare_cxx_compiler_version)
  endif()
  string(REGEX REPLACE "\n.*" "" __mare_cxx_compiler_version "${__mare_cxx_compiler_version}")
endif(MSVC)

if(NOT ${__mare_cxx_compiler_version} STREQUAL "4.8")
  # if the versions do not match, try at least to see if a test app links
  message("Compiler version mismatch, checking if it still works")
  include(CheckCXXSourceCompiles)
  set(__required_flags_backup "${CMAKE_REQUIRED_FLAGS}")
  if(NOT MSVC)
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${MARE_CXX_FLAGS} -L${MARE_CMAKE_DIR}/../..//../lib")
  else()
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${MARE_CXX_FLAGS}")
  endif()
  set(__required_includes_backup "${CMAKE_REQUIRED_INCLUDES}")
  set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES} ${MARE_INCLUDE_DIRS}")
  set(__required_libraries_backup "${CMAKE_REQUIRED_LIBRARIES}")
  set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} ${MARE_LIBRARIES}")

  check_cxx_source_compiles("#include <mare/mare.h>\n #include <stdio.h>\n int main() { mare::runtime::init();\n auto hello = mare::create_task([]{ printf(\"Hello World\"); });\n mare::launch(hello);\n mare::wait_for(hello);\n mare::runtime::shutdown();\n return 0; }" __mare_check_compiler_version)
  if(__mare_check_compiler_version)
    message("While the compiler versions do not match, they seem to be compatible")
  else()
    message(FATAL_ERROR "Compiler versions do not match. MARE was compiled with \"4.8\", please try to match that version.")
  endif()
  set(CMAKE_REQUIRED_FLAGS ${__required_flags_backup})
  set(CMAKE_REQUIRED_INCLUDES ${__required_includes_backup})
  set(CMAKE_REQUIRED_LIBRARIES ${__required_libraries_backup})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MARE  DEFAULT_MSG
  MARE_LIBRARIES MARE_INCLUDE_DIRS MARE_VERSION)
mark_as_advanced(MARE_INCLUDE_DIRS MARE_LIBRARIES)
