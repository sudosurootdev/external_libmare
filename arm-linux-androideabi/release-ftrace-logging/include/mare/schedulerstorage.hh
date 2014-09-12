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
/** @file schedulerstorage.hh */
#pragma once

#include <mare/common.hh>
#include <mare/exceptions.hh>

#include <mare/scopedstorage.hh>
#include <mare/internal/schedulerstorage.hh>

namespace mare {

/** @addtogroup scheduler_storage
@{ */
/**
    Scheduler-local storage allows sharing of information on a
    per-task basis, similar as thread-local storage does for threads.
    A `scheduler_storage_ptr\<T\>` stores a pointer-to-T (`T*`).  In
    contrast to `task_storage_ptr`, the contents are persistent
    between tasks.  In contrast to `thread_storage_ptr`, the contents
    are guaranteed to be unchanged across context switches.  To
    maintain these guarantees, the runtime system is free to create
    new objects of type T whenever needed.

    @sa task_storage_ptr
    @sa thread_storage_ptr

    @par Example
    @includelineno schedstorage01.cc
    @includelineno schedstorage02.cc
    @includelineno schedstorage03.cc
*/
template<typename T, class Allocator = std::allocator<T> >
class scheduler_storage_ptr :
    public scoped_storage_ptr<scheduler_storage_ptr, T, Allocator>
{
public:
  typedef Allocator allocator_type;
  scheduler_storage_ptr() {}
#if __cplusplus >= 201103L
private:
  friend scoped_storage_ptr< ::mare::scheduler_storage_ptr, T, Allocator>;
#else
public:
#endif
  inline static int key_create(internal::storage_key* key,
                               void (*dtor)(void*)) {
    return sls_key_create(key, dtor);
  }
  inline static int set_specific(internal::storage_key key,
                                 void const* value) {
    return sls_set_specific(key, value);
  }
  inline static void* get_specific(internal::storage_key key) {
    return sls_get_specific(key);
  }
};
/** @} */ /* end_addtogroup scheduler_storage */

}; // namespace mare
