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

#include <stdint.h>

#include <mare/internal/tlsptr.hh>

namespace mare {
namespace internal {

class task;

namespace tls {

//
// We put thread specific information here.  We try to make sure that
// every thread we see has this information.  Foreign threads (threads
// that push a task that MARE does not control) may not have any
// information.
//
// The class is aranged this way so you can get the pointer once and
// reuse it locally with no penalty.
//
class info {
public:
  info() : _task(nullptr), _thread_id(), _main_thread(false) {}
  MARE_DEFAULT_METHOD(~info());
  MARE_DELETE_METHOD(info(const info&));
  MARE_DELETE_METHOD(info& operator=(const info&));

  task* current_task() const {return _task;}
  task* set_current_task(task* t) {
    auto prev = _task;
    _task = t;
    return prev;
  }

  uintptr_t thread_id() const { return _thread_id; }
  void set_thread_id(uintptr_t tid) { _thread_id = tid; }

  bool is_main_thread() const { return _main_thread; }
  void set_main_thread() { _main_thread = true; }

private:
  mare::internal::task* _task;
  uintptr_t _thread_id;
  bool _main_thread;
};

// protoype for the TLS pointer
extern tlsptr<info, storage::owner> g_tls_info;

// Static methods for single use
info* init();
void error(std::string msg, const char* filename, int lineno,
           const char* funcname);

// This will get you the pointer so you can use it locally
inline info*
get()
{
  auto ti = g_tls_info.get();
  if (ti)
    return ti;

  return init();
}


} // namespace tls

inline uintptr_t
thread_id()
{
  auto ti = tls::get();
  return ti->thread_id();
}

inline void
set_main_thread()
{
  auto ti = tls::get();
  ti->set_main_thread();
}


} // namespace internal
} // namespace mare
