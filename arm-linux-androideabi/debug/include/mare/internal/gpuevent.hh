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
/** @file gpuevent.hh */
#pragma once

#ifdef HAVE_OPENCL

#include <mare/internal/gpuopencl.hh>

namespace mare {

namespace internal {

class gpuevent
{
private:
  cl::Event _ocl_event;
public:
  gpuevent() : _ocl_event() { }
  explicit gpuevent(const cl::Event& event) : _ocl_event(event) { }
  inline cl::Event get_impl() {
    return _ocl_event;
  }
  inline void wait() {
     _ocl_event.wait();
  }
};

}// ns internal

}// ns mare

#endif //HAVE_OPENCL
