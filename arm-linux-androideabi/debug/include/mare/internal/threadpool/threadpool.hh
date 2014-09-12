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

// @todo size policy

#include <climits>
#include <cstddef>
#include <cstdio>

#include <condition_variable>
#include <future>
#include <functional>
#include <mutex>
#include <set>
#include <thread>
#include <utility>
#include <chrono>

#include <mare/internal/compat.h>
#include <mare/internal/task.hh>
#include <mare/internal/runtime.hh>
#include <mare/internal/semaphore.hh>
#include <mare/internal/debug.hh>

#include <mare/internal/threadpool/storage.hh>
#include <mare/internal/threadpool/scheduler.hh>
#include <mare/internal/threadpool/scheduler-extending.hh>
#include <mare/internal/threadpool/poolthread.hh>
#include <mare/internal/threadpool/locked-lifo.hh>

namespace {

MARE_GCC_IGNORE_BEGIN("-Weffc++");

// template below does not like std::set's second template argument
template<typename T>
struct set : std::set<T> {
};

MARE_GCC_IGNORE_END("-Weffc++");

}; // namespace

namespace mare {

namespace internal {

template<typename Task = std::function<void()>,
         template<typename> class PoolThread = pool_thread,
         template<typename> class Scheduler = pool_extending_scheduler,
         template<typename> class Storage = locked_lifo,
         template<typename> class Collection = set
         >
class threadpool {
public:
  typedef Task task_t;
  typedef PoolThread<threadpool> pool_thread_t;
  typedef Scheduler<threadpool> scheduler_t;
  typedef Storage<pool_thread_t> storage_t;
  typedef Collection<pool_thread_t*> collection_t;
  typedef void (*callback_t)(pool_thread_t&);
  // threads register in one of the following ways:
  // 1. when starting a new thread by the scheduler;
  // 2. when growing the pool

  threadpool() :
    _scheduler(*static_cast<typename scheduler_t::threadpool_t*>(this)),
    _threads(),
    _max_idle_threads(INT_MAX),
    _mutex(),
    _cv(),
    _atstart([](pool_thread_t&){}),
    _atexit([](pool_thread_t&){}) {}

  virtual ~threadpool() {
    stop();
  }

  MARE_DELETE_METHOD(threadpool(threadpool const& other));
  MARE_DELETE_METHOD(const threadpool& operator=(threadpool const&));

public:
  void schedule(task_t&& task) {
    _scheduler.schedule(std::forward<task_t>(task));
  }

  void idle(pool_thread_t& pool_thread) {
    pool_thread.set_idle();
    _scheduler.idle(pool_thread);
  }

  template<typename UnaryFn>
  void for_each(UnaryFn&& fn) {
    _scheduler.for_each(std::forward<UnaryFn>(fn));
  }

  void set_max_idle_threads(size_t max_idle_threads) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (max_idle_threads >= INT_MAX)
      _max_idle_threads = mare::internal::num_cpu_cores();
    else
      _max_idle_threads = max_idle_threads;
  }

  // get the number of idle threads
  size_t get_size() {
    return _scheduler.get_size();
  }

  void wait(pool_thread_t& pool_thread) {
    pool_thread.wait();
  }

  // Remove extra idle threads
  void shrink_idle_threadpool(pool_thread_t* current) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_max_idle_threads < INT_MAX) { // max_idle_threads was set
      size_t n_idle_threads = get_size();
      size_t count = 0;
      if (n_idle_threads > _max_idle_threads) {
        count = n_idle_threads - _max_idle_threads;
        while (count > 0) {
          pool_thread_t* pool_thread = nullptr;
          if (!_scheduler.dequeue(pool_thread)) {
            break;
          }
          delete pool_thread;
          --count;
        }
      }
    }
    // assume that user wants to keep (>=) INT_MAX idle threads

    // register myself as an idle thread and enqueue to the pool
    idle(*current);
    _cv.notify_all();
  }

public:
  //1. resize(0) is a special case and it is for shutting down the thread pool.
  //2. multiple calls to resize() concurrently results undefined behavior.
  void resize(size_t nthreads) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_max_idle_threads >= INT_MAX) // user didn't set max_idle_threads
      _max_idle_threads = mare::internal::num_cpu_cores();

    _max_idle_threads = std::min(nthreads, _max_idle_threads);

    ssize_t delta = nthreads - _threads.size();
    if (delta > 0) {
      grow_unlocked(delta);
    } else {
      auto cv_wait = [this, &lock] {this->_cv.wait(lock);};
      shrink_unlocked(size_t(-delta), cv_wait);
    }
  }

  void grow(ssize_t delta) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_max_idle_threads >= INT_MAX) // user didn't set max_idle_threads
      _max_idle_threads = mare::internal::num_cpu_cores();

    _max_idle_threads = std::min<ssize_t>(_threads.size() + delta,
                                          _max_idle_threads);
    if (ssize_t(_max_idle_threads) < 0)
      _max_idle_threads = mare::internal::num_cpu_cores();

    if (delta >= 0) {
      grow_unlocked(delta);
    } else {
      auto cv_wait = [this, &lock] {this->_cv.wait(lock);};
      shrink_unlocked(size_t(-delta), cv_wait);
    }
  }

  void grow_unlocked(size_t delta) {
    while (delta > 0) {
      add_thread_unlocked();
      --delta;
    }
  }

  template<typename NullaryFn>
  void shrink_unlocked(size_t delta, NullaryFn& cv_wait) {
    size_t count = std::min(delta, _threads.size());
    while (count > _max_idle_threads) {
      pool_thread_t* pool_thread = nullptr;
      while(!_scheduler.dequeue(pool_thread)) {
        if (_threads.size() > _max_idle_threads)
          cv_wait();
        else
          break;
      }
      if (pool_thread != nullptr) {
        delete pool_thread;
        --count;
      }
      count = std::min(count, _threads.size());
    }
  }

  void stop() {
    resize(0);
  }

public:
  void add_thread(task_t&& task) {
    std::unique_lock<std::mutex> lock(_mutex);
    auto pt = new pool_thread_t(*this);
    pt->start(std::forward<task_t>(task));
  }

public: // only to be called from pool_thread_t
  // must only be called within the extent of a locked _mutex.
  // currently, called only by the constructor of pool_thread_t, which
  // is called only from add_thread or add_thread_unlocked (via
  // grow_unlocked).
  void register_thread(pool_thread_t* pool_thread) {
    _threads.insert(pool_thread);
  }

  // must only be called within the extent of a locked _mutex.
  // currently, called only by the destructor of pool_thread_t,
  // which is called only from grow_unlocked.
  void deregister_thread(pool_thread_t* pool_thread) {
    _threads.erase(pool_thread);
  }

protected:
  void add_thread_unlocked() {
    auto pt = new pool_thread_t(*this);
    pt->start();
  }

public:
  template<typename UnaryFn>
  void with_threads(UnaryFn&& fn) {
    std::lock_guard<std::mutex> lock(_mutex);
    fn(_threads);
  }

public: // should not be called after resize(N), N > 0
  template <typename UnaryFn>
  void set_atstart(UnaryFn&& fn) {
    _atstart = std::function<callback_t>(std::forward<UnaryFn>(fn));
  }
  template <typename UnaryFn>
  void set_atexit(UnaryFn&& fn) {
    _atexit = std::function<callback_t>(std::forward<UnaryFn>(fn));
  }

public: // only to be called from pool_thread_t
  void atstart(pool_thread_t& pool_thread) {
    _atstart(pool_thread);
  }
  void atexit(pool_thread_t& pool_thread) {
    _atexit(pool_thread);
  }

protected:
  scheduler_t _scheduler;
  collection_t _threads;
  size_t _max_idle_threads;
  std::mutex _mutex;          // protect _threads, _max_idle_threads
  std::condition_variable _cv;
  callback_t _atstart;
  callback_t _atexit;
};

}; // namespace internal

}; // namespace mare
