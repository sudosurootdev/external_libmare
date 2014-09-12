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

namespace mare {

namespace internal {

template<typename ThreadPool>
class scheduler_interface {
public:
  typedef ThreadPool threadpool_t;
  typedef typename threadpool_t::task_t task_t;
  typedef typename threadpool_t::pool_thread_t pool_thread_t;
  typedef typename threadpool_t::storage_t storage_t;

  scheduler_interface() : _storage() {}

  virtual ~scheduler_interface() {}
  void schedule(task_t&&);
  void idle(pool_thread_t&);
  pool_thread_t* dequeue();             /// blocking dequeue
  bool dequeue(pool_thread_t*& thread); /// non-blocking dequeue
  void wait();
  template<typename UnaryPred> void for_each(UnaryPred&& pred);

protected:
  storage_t& storage() {
    return _storage;
  }

  storage_t _storage;
};

}; // namespace internal

}; // namespace mare
