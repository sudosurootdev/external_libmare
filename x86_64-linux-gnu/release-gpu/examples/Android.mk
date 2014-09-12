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

define mare_add_example
    include $(CLEAR_VARS)
    LOCAL_ARM_MODE := arm
    LOCAL_MODULE := mare_examples_$1
    LOCAL_CPP_EXTENSION := .cc
    LOCAL_CPP_FEATURES := rtti exceptions
    LOCAL_STATIC_LIBRARIES := libmare
    LOCAL_MODULE_TAGS := eng
    LOCAL_SRC_FILES :=
    LOCAL_SRC_FILES += examples/$1.cc
    include $(BUILD_EXECUTABLE)
endef

define mare_add_example_shared
    include $(CLEAR_VARS)
    LOCAL_ARM_MODE := arm
    LOCAL_MODULE := sharedmare_examples_$1
    LOCAL_CPP_EXTENSION := .cc
    LOCAL_CPP_FEATURES := rtti exceptions
    LOCAL_SHARED_LIBRARIES := libmare_shared
    LOCAL_STATIC_LIBRARIES :=
    LOCAL_MODULE_TAGS := eng
    LOCAL_SRC_FILES :=
    LOCAL_SRC_FILES += examples/$1.cc
    include $(BUILD_EXECUTABLE)
endef

example_names :=             \
	abort-on-cancel      \
	after                \
	attrblocking         \
	attrlongrunning      \
	cancel-group         \
	dom-styling1         \
	dom-styling2         \
	helloworld1          \
	mm                   \
	sdfadvanced          \
	sdfadvanceddebug     \
	sdfassigncost        \
	sdfasynclaunch       \
	sdfbasicpipe         \
	sdfbodytypes         \
	sdffilter            \
	sdffiltergeneralized \
	sdfpauseresumecancel \
	sdfprogrammatic      \
	storage1

ifeq ($(MARE_HAVE_GPU),1)
  example_names += vector-add-gpu vector-add-gpu-pfor-each
endif

ifeq ($(APP_STL),gnustl_static)
  $(foreach ex,$(example_names),$(eval $(call mare_add_example,$(ex))))
endif
ifeq ($(APP_STL),gnustl_shared)
  $(foreach ex,$(example_names),$(eval $(call mare_add_example_shared,$(ex))))
endif

