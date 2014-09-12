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

#include <memory>

#include <mare/internal/semaphore.hh>
#include <mare/internal/tls.hh>

namespace mare {

namespace internal {

template<typename ThreadPool>
class pool_thread {
public:
  typedef ThreadPool threadpool_t;
  typedef typename threadpool_t::task_t task_t;

  pool_thread(threadpool_t& pool)
    : _pool(pool),
      _task(),
      _task_valid(false),
      _ready(false),
      _sem(),
      _thread()
  {
    _pool.register_thread(this_crtp());
  }

  virtual ~pool_thread() {
    stop();
    join();
    _pool.deregister_thread(this_crtp());
  }

  MARE_DELETE_METHOD(pool_thread(pool_thread const&));
  MARE_DELETE_METHOD(const pool_thread& operator=(pool_thread const&));

  void operator()(bool task_available = false) {
    _pool.atstart(*this_crtp());
    if (!task_available && ready())
      goto idle;
    while (ready()) {
      this_crtp()->execute();
    idle:
      // before going to idle, remove extra idle threads
      _pool.shrink_idle_threadpool(this_crtp());
      // task ownership released
      _pool.wait(*this_crtp());
      // task ownership acquired
    }
    _pool.atexit(*this_crtp());
  }

  void wait() {
    _sem.wait(); // async_exec is only to be called within this region
  }

  void set_idle() {
    _task_valid = false;
  }

  // caller must have ownership of task
  void execute() {
    _task();
  }

  void start() {
    _ready = true;
    _thread = std::unique_ptr<std::thread>
      (new std::thread(&bootstrap,
                       this_crtp(), false));
  }

  void start(task_t&& task) {
    _task = std::move(task);     // ok not to lock, thread not running yet
    _task_valid = true;
    _ready = true;
    _thread = std::unique_ptr<std::thread>(new std::thread(&bootstrap,
                                                           this_crtp(), true));
  }

  // caller must have ownership of pool_thread, i.e., this thread must
  // not have ownership of any other task
  void async_exec(task_t&& task) {
    _sem.signal([&] {
        this->_task = std::move(task);
        this->_task_valid = true;
      });
  }

  bool ready() const {
    return _ready;
  }

  void stop() {
    _ready = false;
    _sem.signal();
  }

protected:
  void join() {
    _thread->join();
  }

  static void bootstrap(typename threadpool_t::pool_thread_t* pt,
                        bool task_available) {
    tls::init();

    (*pt)(task_available);

#ifdef _MSC_VER
    // On Windows, if you let a thread unwind and exit normally, it
    // grabs a lock to process the exit code. However, if you are in
    // shutdown() called during the atexit() handler in the main
    // thread, it will terminate the pool threads, and the pool
    // threads will end up here. But if you go beyond this point, it
    // will try to grab the exit mutex, and deadlock MARE. So by force
    // exiting our thread here, we avoid any exit mutex. This should
    // be ok, because nothing should be cleaned up during the exit
    // handler here any way, the main thread takes care of all this
    // for us.
    ExitThread(0);
#endif // _MSC_VER
  }

  auto this_crtp() -> typename threadpool_t::pool_thread_t* {
    return static_cast<typename threadpool_t::pool_thread_t*>(this);
  }

protected:
  threadpool_t& _pool;
  task_t _task;
  bool _task_valid;
  bool _ready;
  semaphore _sem; // protects/signals whether _task is ready for execution
  std::unique_ptr<std::thread> _thread;
};

}; // namespace internal

}; // namespace mare
