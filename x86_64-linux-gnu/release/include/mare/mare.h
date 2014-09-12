// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
// Copyright 2013 Qualcomm Technologies, Inc.  All rights reserved.
// Confidential & Proprietary – Qualcomm Technologies, Inc. ("QTI")
// 
// The party receiving this software directly from QTI (the "Recipient")
// may use this software as reasonably necessary solely for the purposes
// set forth in the agreement between the Recipient and QTI (the
// "Agreement").  The software may be used in source code form solely by
// the Recipient's employees (if any) authorized by the Agreement.
// Unless expressly authorized in the Agreement, the Recipient may not
// sublicense, assign, transfer or otherwise provide the source code to
// any third party.  Qualcomm Technologies, Inc. retains all ownership
// rights in and to the software.
// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
/** @file mare.h */
#pragma once

#include <mare/buffer.hh>
#include <mare/device.hh>
#include <mare/gpukernel.hh>
#include <mare/gputask.hh>
#include <mare/group.hh>
#include <mare/index.hh>
#include <mare/range.hh>
#include <mare/runtime.hh>
#include <mare/task.hh>

#include <mare/internal/compat.h>
#include <mare/internal/taskfactory.hh>

extern "C" const char *mare_version();
extern "C" const char *mare_commit();

