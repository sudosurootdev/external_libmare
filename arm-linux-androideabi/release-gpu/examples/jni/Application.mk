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
# This Application.mk is provided to allow quick testing of MARE
# building with ndk-build, and generates the mare_helloworld
# example. You would not normally use this file. See the MARE
# documentation for instructions on integrating MARE into your
# own Android application.

APP_STL := gnustl_static
APP_ABI := armeabi-v7a
NDK_TOOLCHAIN_VERSION := 4.8
#set the APP_PLATFORM to match your platform version.
#APP_PLATFORM := android-19
LOCAL_ARM_NEON := true
