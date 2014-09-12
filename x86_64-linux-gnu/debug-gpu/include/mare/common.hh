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
/** @file common.hh */
#pragma once

#include <condition_variable>

#include <mare/internal/macros.hh>
#include <mare/internal/mareptrs.hh>
#include <mare/internal/sparse-bitmap.hh>

namespace mare {

#ifdef HAVE_OPENCL
template<typename T>
class buffer;

template<typename...Kargs>
class gpukernel;
#endif // HAVE_OPENCL

namespace internal {
class group;
class task;
#ifdef HAVE_OPENCL
class gpudevice;
class gpubuffer;
#endif // HAVE_OPENCL

typedef sparse_bitmap group_signature;
static group_signature empty_bitmap = sparse_bitmap(0);

//Empty base class used for with_attrs_dispatcher
class gpu_kernel_base { };

} //namespace internal

/** @cond IGNORE_SHARED_PTR */
// An  std::shared_ptr-like managed pointer to a group object.
typedef internal::mare_shared_ptr<internal::group> group_ptr;

// An std::shared_ptr-like managed pointer to a task object.
typedef internal::mare_shared_ptr<internal::task> task_ptr;

#ifdef HAVE_OPENCL
// An std::shared_ptr-like managed pointer to a device object.
typedef internal::mare_shared_ptr<internal::gpudevice> device_ptr;

// An std::share_ptr-like managed pointer to a gpubuffer object.
typedef internal::mare_shared_ptr<internal::gpubuffer> gpubuffer_ptr;

template<typename T>
using buffer_ptr = internal::mare_buffer_ptr< ::mare::buffer<T> >;

template<typename... Kargs>
using kernel_ptr = internal::mare_shared_ptr< ::mare::gpukernel<Kargs...> >;
#endif // HAVE_OPENCL

// A raw -like pointer to a task object.o
typedef internal::mare_unsafe_ptr<internal::task> unsafe_task_ptr;


typedef
#if defined _MSC_FULL_VER && (_MSC_FULL_VER == MARE_VS2012_NOV_CTP)
std::cv_status::cv_status
#else
std::cv_status
#endif // _MSC_FULL_VER == MARE_VS2012_NOV_CTP
cv_status;
/** @endcond */

}; //namespace

