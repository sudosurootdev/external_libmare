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
#pragma once

#ifdef HAVE_OPENCL

#include <mare/internal/gpukernel.hh>

namespace mare {


template<typename...Params>
kernel_ptr<Params...>
create_kernel(const std::string& kernel_str, const std::string& kernel_name)
{
  auto k_ptr = new gpukernel<Params...>(kernel_str, kernel_name);
  MARE_DLOG("Creating mare::gpukernel: %s", kernel_name.c_str());
  return kernel_ptr<Params...>(k_ptr,
                              kernel_ptr<Params...>::ref_policy::
                              DO_INITIAL_REF);
}

}//ns mare

#endif //HAVE_OPENCL
