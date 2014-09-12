# --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
# Copyright 2013 Qualcomm Technologies, Inc.  All rights reserved.
# Confidential & Proprietary – Qualcomm Technologies, Inc. ("QTI")
# 
# The party receiving this software directly from QTI (the "Recipient")
# may use this software as reasonably necessary solely for the purposes
# set forth in the agreement between the Recipient and QTI (the
# "Agreement").  The software may be used in source code form solely by
# the Recipient's employees (if any) authorized by the Agreement.
# Unless expressly authorized in the Agreement, the Recipient may not
# sublicense, assign, transfer or otherwise provide the source code to
# any third party.  Qualcomm Technologies, Inc. retains all ownership
# rights in and to the software.
# --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
cmake_minimum_required(VERSION 2.8.7)

project (MAREExamples)

# MARE_DIR points to the directory where MARE was installed
if(NOT MARE_DIR)
  if(NOT "$ENV{MARE_DIR}" STREQUAL "")
    set(MARE_DIR $ENV{MARE_DIR})
  else()
    set(MARE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
  endif()
endif()

find_package(MARE HINTS ${MARE_DIR}/lib/CMake/mare ${MARE_DIR}/CMake
  NO_DEFAULT_PATH
)

if(MARE_FOUND)
  message("MARE version: ${MARE_VERSION_STRING}")
  message("MARE includes: ${MARE_INCLUDE_DIRS}")
  message("MARE cxx flags: ${MARE_CXX_FLAGS}")
  message("MARE libs: ${MARE_LIBRARIES}")
else()
  message(FATAL_ERROR "MARE not found")
endif()

include_directories(${MARE_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MARE_CXX_FLAGS}")

macro(mare_add_example __example __example_SRCS)
  add_executable(${__example} ${__example_SRCS})
  target_link_libraries(${__example} ${MARE_LIBRARIES})
endmacro(mare_add_example)


mare_add_example(abort-on-cancel abort-on-cancel.cc)

mare_add_example(after after.cc )

mare_add_example(attrblocking attrblocking.cc)

mare_add_example(attrlongrunning attrlongrunning.cc)

mare_add_example(cancel-group cancel-group.cc)

mare_add_example(dom-styling1 dom-styling1.cc)

mare_add_example(dom-styling2 dom-styling2.cc)

mare_add_example(helloworld1 helloworld1.cc)

mare_add_example(mm mm.cc)

mare_add_example(sdfadvanced sdfadvanced.cc)

mare_add_example(sdfadvanceddebug sdfadvanceddebug.cc)

mare_add_example(sdfassigncost sdfassigncost.cc)

mare_add_example(sdfasynclaunch sdfasynclaunch.cc)

mare_add_example(sdfbasicpipe sdfbasicpipe.cc)

mare_add_example(sdfbodytypes sdfbodytypes.cc)

mare_add_example(sdffilter sdffilter.cc)

mare_add_example(sdffiltergeneralized sdffiltergeneralized.cc)

mare_add_example(sdfpauseresumecancel sdfpauseresumecancel.cc)

mare_add_example(sdfprogrammatic sdfprogrammatic.cc)

mare_add_example(storage1 storage1.cc)

if(MARE_HAVE_GPU)
  mare_add_example(vector-add-gpu vector-add-gpu.cc)
  
  mare_add_example(vector-add-gpu-pfor-each vector-add-gpu-pfor-each.cc)
endif(MARE_HAVE_GPU)

