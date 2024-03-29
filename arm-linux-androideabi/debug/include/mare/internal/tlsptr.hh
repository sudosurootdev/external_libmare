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

#include <errno.h>
#include <string.h>

#include <memory>

#include <mare/exceptions.hh>
#include <mare/internal/compat.h>

namespace mare {

namespace internal {

namespace tls {
void error(std::string msg, const char* filename, int lineno,
           const char* funcname);
}

namespace storage {

/// \brief a box which does not assume ownership of its pointer
///
/// It is expected that the stored pointer is disposed by someone
/// else.
template <typename T>
struct box {
  T* _ptr;
  box(T* ptr) : _ptr(ptr) {};
  MARE_DELETE_METHOD(box(box const&));
  MARE_DELETE_METHOD(box& operator=(box const&));
  T* get() const {
    return _ptr;
  }
  box& operator=(T* const& ptr) {
    _ptr = ptr;
    return *this;
  }
};

/// \brief a box which assumes ownership of its pointer
// this should really by std::unique_ptr<T>, but the interface is slightly off
template <typename T>
struct owner {
  T* _ptr;
  owner(T* ptr) : _ptr(ptr) {};
  MARE_DELETE_METHOD(owner(owner const&));
  MARE_DELETE_METHOD(owner& operator=(owner const&));
  ~owner() {
    if (_ptr)
      delete _ptr;
  }
  T* get() const {
    return _ptr;
  }
  owner& operator=(T* const& ptr) {
    if (_ptr != nullptr)
      delete _ptr;
    _ptr = ptr;
    return *this;
  }
};

}; // namespace storage

template<typename T, template <typename> class Box = storage::box>
class tlsptr {
public:
  typedef T* pointer_type;

  MARE_DELETE_METHOD(tlsptr(tlsptr const&));
  MARE_DELETE_METHOD(tlsptr& operator=(tlsptr const&));
#ifndef _MSC_VER
  MARE_DELETE_METHOD(tlsptr& operator=(tlsptr const&) volatile);
#endif

  tlsptr(): _key(), _err() {
    initKey();
  }

  tlsptr(T* const& ptr)
    : _key(), _err() {
    initKey();
    *this = ptr;
  }

  ~tlsptr() {
    if (_err != 0) {
      // pthread_key_create failed, do not try to delete the key.
      return;
    }
    if (int err = pthread_key_delete(_key)) {
      // key is deleted here, so platforms which use TLS in exception
      // handling should have no problems throwing the exception.
      switch(err) {
      case EINVAL:
        throw mare::tls_exception("The specified argument is not correct.",
                                  __FILE__, __LINE__, __FUNCTION__);
      case ENOENT:
        throw mare::tls_exception("Specified TLS key is not allocated.",
                                  __FILE__, __LINE__, __FUNCTION__);
      default:
        throw mare::tls_exception("Unknown error.", __FILE__,
                                  __LINE__, __FUNCTION__);
      }
    }
  }

  T* get() const {
    if (auto p = pthread_getspecific(_key)) {
      return static_cast<Box<T>*>(p)->get();
    }
    return pointer_type();
  }

  T* operator->() {
    return get();
  }

  T& operator*() {
    return *get();
  }

  tlsptr& operator=(T* const& ptr) {
    // Avoid calling pthread_getspecific() on each assignment, because
    // it is expensive on some platforms (e.g., Android).  Instead, we
    // store a "box" in TLS.  Once it exists, we can obtain it with
    // pthread_getspecific, and update its value.
    if (auto p = pthread_getspecific(_key)) {
      *static_cast<Box<T>*>(p) = ptr;
    } else {
      int rc = pthread_setspecific(_key, new Box<T>(ptr));
      if (rc != 0) {
        std::string msg =
          strprintf("Call to pthread_setspecific() failed [%d]:%s",
                    rc,
                    mare_strerror(rc).c_str());
        tls::error(msg, __FILE__, __LINE__, __FUNCTION__);
      }
    }
    return *this;
  }

  bool operator!() {
    pointer_type t = get();
    return t==nullptr;
  }

  bool operator==(T* const& other) {
    return get()==other;
  }

  bool operator!=(T* const& other) {
    return get()!=other;
  }

  operator pointer_type() const {
    return get();
  }

  operator bool() const {
    return get()!=nullptr;
  }

  const pthread_key_t& key() {
    return _key;
  }

  int get_error() const {
    return _err;
  }

private:
  static void delete_box(void* ptr) {
    auto boxptr = static_cast<Box<T>*>(ptr);
    if (boxptr)
      delete boxptr;
  }

  void initKey() {
    _err = pthread_key_create(&_key, delete_box);
    // We cannot throw an exception here if _err != 0, because some
    // platforms (MacOS X) use TLS in exception handling, and if all
    // TLS is used up, this will hang or crash.
  }
private:
  pthread_key_t _key;
  int _err;
};
} // namespace internal
} // namespace mare
