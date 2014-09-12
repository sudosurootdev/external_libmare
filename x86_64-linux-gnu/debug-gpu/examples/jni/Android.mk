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
LOCAL_PATH := $(PWD)/..

#ANDROID_CL_DIR := <path to Android OpenCL Includes containing patched cl.hpp and lib directory containing libOpenCL.so>

include $(LOCAL_PATH)/lib/MARE.mk
include $(LOCAL_PATH)/examples/Android.mk
