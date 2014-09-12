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
/** @file device.hh */
#pragma once

#ifdef HAVE_OPENCL

#include <mare/common.hh>

#include <mare/internal/gpudevice.hh>

namespace mare {

template<typename Attribute>
inline void get_devices(Attribute d_type, std::vector<device_ptr>* devices)
{
  MARE_INTERNAL_ASSERT(devices, "null vector passed to get_devices");
  internal::get_devices(d_type, devices);
}

//get(mare::device_attr::endian_little)
//Attribute will be deduced as internal::device_attr_endian_little.
//it would use the specialization
//internal::device_attr<device_attr_endian_little>
template<typename Attribute>
auto get(device_ptr device, Attribute attr) ->
     typename internal::device_attr<Attribute>::type
{
  auto d_ptr = internal::c_ptr(device);
  MARE_API_ASSERT(d_ptr, "null device_ptr");
  return d_ptr->get(std::forward<Attribute>(attr));
}

} //ns mare
#endif // HAVE_OPENCL
