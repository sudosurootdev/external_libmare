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
/** @file gpuopencl.hh */
#pragma once

#ifdef HAVE_OPENCL

#include <mare/internal/debug.hh>

#define __CL_ENABLE_EXCEPTIONS
MARE_GCC_IGNORE_BEGIN("-Wshadow");
MARE_GCC_IGNORE_BEGIN("-Weffc++");
MARE_GCC_IGNORE_BEGIN("-Wold-style-cast");
MARE_GCC_IGNORE_BEGIN("-Wunused-parameter");

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

MARE_GCC_IGNORE_END("-Wunused-parameter");
MARE_GCC_IGNORE_END("-Wold-style-cast");
MARE_GCC_IGNORE_END("-Weffc++");
// we have to leave this one on.
//MARE_GCC_IGNORE_END("-Wshadow");

#endif // HAVE_OPENCL

