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
MARE_LIB_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
MARE_CXX_FLAGS :=  -std=c++11 -O3 -DNDEBUG
ifeq (1,1)
  MARE_HAVE_GPU := 1
  MARE_CXX_FLAGS += -DHAVE_OPENCL -DHAVE_CLRETAINDEVICE
  ANDROID_CL_LIB := -lOpenCL
  ANDROID_CL_LDFLAGS := -L$(ANDROID_CL_DIR)/lib
  ANDROID_CL_INCLUDE_DIR := $(ANDROID_CL_DIR)/include
else
  MARE_HAVE_GPU := 0
endif

ifeq (RELEASE,RELEASE)
  APP_OPTIM := release
else
  APP_OPTIM := debug
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libmare
LOCAL_EXPORT_C_INCLUDES := $(MARE_LIB_DIR)/../include $(ANDROID_CL_INCLUDE_DIR)
LOCAL_EXPORT_CPPFLAGS := $(MARE_CXX_FLAGS)
LOCAL_EXPORT_LDFLAGS := $(ANDROID_CL_LDFLAGS)
LOCAL_EXPORT_LDLIBS := -llog $(ANDROID_CL_LIB)
LOCAL_SRC_FILES := $(MARE_LIB_DIR)/libmare.a
include $(PREBUILT_STATIC_LIBRARY)



