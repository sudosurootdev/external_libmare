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

#include <mutex>

#include <mare/internal/threadpool/threadpool.hh>
#include <mare/internal/debug.hh>

namespace mare {

namespace internal {

template<typename ThreadPool>
class locked_pool_thread : public pool_thread<ThreadPool> {
  typedef pool_thread<ThreadPool> super;
public:
  locked_pool_thread(ThreadPool& pool)
    : super(pool)
  {}

  std::mutex& mutex() {
    return super::_sem.mutex();
  }

  template<typename UnaryPred>
  bool with_current_task(UnaryPred&& fn) {
    // GCC 4.6.3 barfs on -> decltype(fn(this->_task))
    if (!this->_task_valid)
      return true;
    std::lock_guard<std::mutex> lock(mutex());
    return fn(this->_task);
  }
};

template<typename ThreadPool>
class locked_scheduler : public pool_extending_scheduler<ThreadPool> {
  typedef pool_extending_scheduler<ThreadPool> super;
public:
  locked_scheduler(ThreadPool& pool)
    : super(pool)
  {}

  template<typename UnaryFn>
  void for_each_current_task(UnaryFn&& fn) {
    super::for_each([&] (typename super::pool_thread_t& thread) {
        return thread.with_current_task([&] (typename super::task_t& task) {
            return fn(task);
          });
      });
  }
};

template<typename Task = std::function<void()>,
         template<typename> class PoolThread = locked_pool_thread,
         template<typename> class Scheduler = locked_scheduler,
         template<typename> class Storage = locked_lifo,
         template<typename> class Collection = set
         >
struct locked_threadpool :
    public threadpool<Task,PoolThread,Scheduler,Storage,Collection>
{
  template<typename UnaryFn>
  void for_each_current_task(UnaryFn&& fn) {
    this->_scheduler.for_each_current_task(std::forward<UnaryFn>(fn));
  }
};

}; // namespace internal

}; // namespace mare
