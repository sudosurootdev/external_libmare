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

#include <mare/internal/threadpool/scheduler.hh>

#include <mare/internal/debug.hh>

namespace mare {

namespace internal {

template<typename ThreadPool>
class pool_extending_scheduler : public scheduler_interface<ThreadPool> {
public:
  typedef typename scheduler_interface<ThreadPool>::threadpool_t threadpool_t;
  typedef typename scheduler_interface<ThreadPool>::pool_thread_t
    pool_thread_t;
  typedef typename scheduler_interface<ThreadPool>::task_t task_t;

  pool_extending_scheduler(threadpool_t& pool)
    : _pool(pool)
  {}

  virtual ~pool_extending_scheduler() {}

  MARE_DELETE_METHOD(
      pool_extending_scheduler(pool_extending_scheduler const&));
  MARE_DELETE_METHOD(const pool_extending_scheduler& operator=
                     (pool_extending_scheduler const&));

  using scheduler_interface<ThreadPool>::storage;
public:
  // thread-safe
  void schedule(task_t&& task) {
    pool_thread_t* pool_thread = nullptr;
    if (storage().dequeue(pool_thread)) {
      MARE_INTERNAL_ASSERT(pool_thread, "invalid null pool_thread");
      pool_thread->async_exec(std::forward<task_t>(task));
    } else {
      // NOTE: pool::add_thread must be thread-safe
      _pool.add_thread(std::forward<task_t>(task));
    }
  }

  void idle(pool_thread_t& pool_thread) {
    storage().enqueue(&pool_thread);
  }

  bool dequeue(pool_thread_t*& thread) {
    return storage().dequeue(thread);
  }

  pool_thread_t* dequeue() {
    return storage().dequeue();
  }

  void wait() {
    return storage().wait();
  }

  template<typename UnaryPred>
  void for_each(UnaryPred&& pred) {
    _pool.with_threads([&] (typename threadpool_t::collection_t& threads) {
        for (auto thread : threads)
          if (!pred(*thread))
            break;
      });
  }

  size_t get_size() {
    return storage().get_size();
  }

protected:
  threadpool_t& _pool;
};

}; // namespace internal

}; // namespace mare
