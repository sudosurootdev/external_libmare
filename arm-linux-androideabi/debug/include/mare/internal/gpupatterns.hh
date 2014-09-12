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
/** @file gpupatterns.hh */
#pragma once

#ifdef HAVE_OPENCL

#include <mare/range.hh>

#include <mare/metatype/distance.hh>

namespace mare {

namespace internal {

//BWAG - Body with attributes GPU
template <typename BWAG>
void pfor_each_gpu(group_ptr group, size_t first, size_t last, BWAG bwag) {

  if (first >= last)
    return;
  auto g_internal = create_group("pfor_each_gpu");
  auto g = g_internal;
  if (group)
    g = g & group;

  auto g_ptr = c_ptr(group);
  if (g_ptr && g_ptr->is_canceled())
    return;

  mare::range<1> r(first, last);
  pfor_each_gpu(g, r, bwag);
}

template <size_t DIMS, typename BWAG>
void pfor_each_gpu(group_ptr, const mare::range<DIMS>& r, BWAG bwag)
{
  typedef typename BWAG::kernel_parameters k_params;
  typedef typename BWAG::kernel_arguments k_args;

  //get devices.
  auto d_ptr = c_ptr(internal::s_dev_ptrs[0]);
  auto k_ptr = c_ptr(bwag.get_gpu_kernel());
  gpukernel* kernel = k_ptr->get_kernel();

  gpu_kernel_dispatch::
    prepare_args<k_params, k_args>(s_dev_ptrs[0],
                                   kernel,
                                   std::forward<k_args>(
                                     bwag.get_kernel_args()));

  //launch kernel.
  gpuevent event = d_ptr->launch_kernel(kernel, r);
  event.wait();
}

} //ns internal

} //ns mare

#endif //HAVE_OPENCL

