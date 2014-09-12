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
/** @file taskstorage.hh */
#pragma once

#include <mare/common.hh>
#include <mare/exceptions.hh>

#include <mare/internal/taskstorage.hh>

namespace mare {

/** @addtogroup task_storage
@{ */
/**
    Task-local storage allows sharing of information on a per-task
    basis, similar as thread-local storage does for threads.  The
    value of a `task_storage_ptr` is local to a task.  A
    `task_storage_ptr\<T\>` stores a pointer-to-T (`T*`).

    @par Example
    @includelineno taskstorage01.cc
*/
template<typename T>
class task_storage_ptr {
public:
  typedef T const* pointer_type;

  MARE_DELETE_METHOD(task_storage_ptr(task_storage_ptr const&));
  MARE_DELETE_METHOD(task_storage_ptr& operator=(task_storage_ptr const&));
#ifndef _MSC_VER
  MARE_DELETE_METHOD(task_storage_ptr& operator=
                     (task_storage_ptr const&) volatile);
#endif

/** @throws mare::tls_exception if `task_storage_ptr` could not be reserved. */
  task_storage_ptr()
    : _key() {
    init_key();
  }

  /**
      @throws mare::tls_exception if `task_storage_ptr` could not be reserved.
      @param ptr initial value of task-local storage.
  */
  task_storage_ptr(T* const& ptr)
    : _key() {
    init_key();
    *this = ptr;
  }

  /**
      @throws mare::tls_exception if `task_storage_ptr` could not be reserved.

      @param ptr initial value of task-local storage.
      @param dtor destructor function.
  */
  task_storage_ptr(T* const& ptr, void (*dtor)(T*))
    : _key() {
    init_key(dtor);
    *this = ptr;
  }

  /** @returns Pointer to stored pointer value. */
  T* get() const {
    return static_cast<T*>(task_get_specific(_key));
  }

  /** @returns Pointer to stored pointer value. */
  T* operator->() const {
    return get();
  }


  /** @returns Reference to value pointed to by stored pointer.

      @note1 No checking for `nullptr` is performed.
  */
  T& operator*() const {
    return *get();
  }

  /**
      Assignment operator, stores T*.

      @param ptr Pointer value to store.
  */
  task_storage_ptr& operator=(T* const& ptr) {
    task_set_specific(_key, ptr);
    return *this;
  }

  /** @returns True if stored pointer is `nullptr`. */
  bool operator!() const {
    pointer_type t = get();
    return t == nullptr;
  }

  bool operator==(T* const& other) const {
    return get() == other;
  }

  bool operator!=(T* const& other) const {
    return get() != other;
  }

  /** Casting operator to `T*` pointer type. */
  operator pointer_type() const {
    return get();
  }

  /** Casting operator to bool. */
  operator bool() const {
    return get() != nullptr;
  }

private:
  void init_key(void (*dtor)(T*) = nullptr) {
    int err = task_key_create(&_key, reinterpret_cast<void(*)(void*)>(dtor));
    if (err != 0)
      throw mare::tls_exception("Unable to allocate task-local storage key.",
                                __FILE__, __LINE__, __FUNCTION__);
  }
private:
  internal::storage_key _key;            /** Stores task-local storage key. */
};

/** @} */ /* end_addtogroup task_storage */

}; // namespace mare
