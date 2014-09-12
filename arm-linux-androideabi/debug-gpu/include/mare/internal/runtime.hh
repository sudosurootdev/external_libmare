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

#include <mare/internal/tls.hh>

namespace mare {

namespace internal {

// Number of cores in the system
extern size_t s_num_cores;
// Number of execution contexts MARE is using
extern size_t s_num_exec_ctx;
extern bool mare_started;
extern bool mare_stopped;

enum class thread_affinity_t : bool {NOT_SET = false, SET = true};
extern thread_affinity_t g_thread_affinity;

void send_to_runtime(task* task);

inline size_t
num_cpu_cores() {
  return s_num_cores;
}
size_t find_number_of_cpu_cores();

inline size_t
num_execution_contexts() {
  return s_num_exec_ctx;
}

inline thread_affinity_t
is_thread_affinity_set() {
  return g_thread_affinity;
}

void cancel_blocking_tasks(group_ptr const& group);

// Do not use mutex here for performance reasons, the only time the mutex
// is needed is for making sure init and shutdown are done atomically.
inline bool runtime_available () {
  return (internal::mare_started && !internal::mare_stopped);
}
inline bool runtime_stopped () {
  return internal::mare_stopped;
}
inline bool runtime_not_stopped () {
  return !internal::mare_stopped;
}

void runtime_init(size_t num_execution_contexts_requested,
        thread_affinity_t thread_affinity = thread_affinity_t::NOT_SET);
void runtime_shutdown();

// Handler called during exit() to call shutdown automatically.
void atexit_handler ();
#ifndef _MSC_VER
#ifndef __ANDROID__

// Handler called during fork() that we use to prevent this call, since fork()
// does not copy the thread pool over, it is important that we never do this
// while MARE is running!
void prefork_handler ();
#endif // __ANDROID__
#endif // _MSC_VER

}; // namespace internal
}; // namespace mare

