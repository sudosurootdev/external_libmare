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

#include <queue>
#include <cstdlib>
#include <atomic>
#include <mutex>
#include <thread>

#include <mare/common.hh>
#include <mare/internal/dualtaskqueue.hh>
#include <mare/internal/task.hh>
#include <mare/internal/blocked_task.hh>
#include <mare/internal/mutex_cv.hh>

namespace mare{

namespace internal{

// tts_spin_lock - test and test and set lock that spins on a global
// atomic bool until it successfully flips the bool to true (for held)
// Currently yields to the MARE scheduler after exceeding a threshold
// of spin iterations

class tts_spin_lock{
private:
  std::atomic<bool> _held;

public:
  tts_spin_lock() : _held(false) {}

  MARE_DELETE_METHOD(tts_spin_lock(tts_spin_lock &));
  MARE_DELETE_METHOD(tts_spin_lock(tts_spin_lock &&));
  MARE_DELETE_METHOD(tts_spin_lock& operator=(tts_spin_lock const &));
  MARE_DELETE_METHOD(tts_spin_lock& operator=(tts_spin_lock&&));

  void lock();
  bool try_lock();
  void unlock();
};

class futex_lock_free_queue{
private:
  // Lock is only used for broadcast wakeup
  tts_spin_lock _lock;
  DualTaskQueue<blocked_task*> _queue;

public:

  futex_lock_free_queue() : _lock(), _queue() {
  }

  void enqueue(blocked_task * task)
  {
    _queue.threadInitAuto();
    _queue.push(task);
  }

  blocked_task* dequeue()
  {
    _queue.threadInitAuto();
    blocked_task* result = nullptr;
    paired_mutex_cv synch;
    if(_queue.popIf([]{ return false; }, result, &synch) == false){
      result = nullptr;
    }
    return result;
  }

  blocked_task* unlocked_dequeue()
  {
    return dequeue();
  }

  tts_spin_lock& get_lock(){
    return _lock;
  }
};

class futex_locked_queue{
private:
  tts_spin_lock _lock;
  std::queue<blocked_task*> _queue;

public:

  futex_locked_queue() : _lock(), _queue(){
  }

  void enqueue(blocked_task * task)
  {
    _lock.lock();
    _queue.push(task);
    _lock.unlock();
  }

  blocked_task* dequeue()
  {
    blocked_task* result = nullptr;
    _lock.lock();
    if(_queue.empty() == false){
      result = _queue.front();
      _queue.pop();
    }
    _lock.unlock();
    return result;
  }

  blocked_task* unlocked_dequeue()
  {
    blocked_task* result = nullptr;
    if(_queue.empty() == false){
      result = _queue.front();
      _queue.pop();
    }
    return result;
  }

  tts_spin_lock& get_lock(){
    return _lock;
  }
};

// Provides wait and wakeup facilities for tasks to
// block within MARE (rather than in the OS) and be
// explicitly woken up by another task
class futex{
private:
  futex_locked_queue _blocked_tasks;
  std::atomic<size_t> _waiters;
  std::atomic<size_t> _awake;

public:
  futex() : _blocked_tasks(), _waiters(0), _awake(0) {
  }

  MARE_DELETE_METHOD(futex(futex&));
  MARE_DELETE_METHOD(futex(futex&&));
  MARE_DELETE_METHOD(futex& operator=(futex const&));
  MARE_DELETE_METHOD(futex& operator=(futex&&));

  // Causes the task to block until another task calls wakeup
  //
  // If this is the first call to wait after a call to
  // wakeup failed to wakeup any tasks, the task will not block
  // This condition prevents races
  // between calls to wait and wakeup from not waking
  // any tasks and allows synchronization primitives
  // to be oblivious of whether or not any tasks are
  // currently waiting on the futex
  void wait(std::atomic<int>* check_value_ptr = nullptr, int check_value = 0);

  std::atomic<int>* timed_wait(std::atomic<int>* check_value_ptr = nullptr,
                               int check_value = 0);

  // Will attempt to wake up at least 1 waiting task
  //
  // If unable to wake up any waiting tasks (due to none
  // waiting or a race between wait and wakeup), wakeup
  // indicates that the next call to wait should not block
  // (i.e., the next task to call wait() is the task that is
  // woken up by the failed call to wakeup(...)
  size_t wakeup(size_t num_tasks);

private:
  // Attempts to wakeup a single task
  //
  // returns 1 if successful, 0 if not
  // param unlocked true Do unlocked access to queue
  //                false Use normal locked access to queue
  //                Used for broadcast, which locks, dequeues all tasks, and
  //                then unlocks, rather than repeatedly locking
  size_t wakeup_one(bool unlocked = false);
};

// Mutex that uses the futex for blocking when a
// lock acquire fails
class mutex{
private:
  std::atomic<int> _state;
  futex _futex;

public:
  // Low threshold because yield() is used to backoff between spins
  static const int SPIN_THRESHOLD = 10;

  mutex() : _state(0), _futex()
  {}

  MARE_DELETE_METHOD(mutex(mutex&));
  MARE_DELETE_METHOD(mutex(mutex&&));
  MARE_DELETE_METHOD(mutex& operator=(mutex const&));
  MARE_DELETE_METHOD(mutex& operator=(mutex&&));

  void lock();
  bool try_lock();
  void unlock();
};

/// Adds an id and number of permits (for that id) to
/// allow a task/thread to call lock and unlock multiple times
template<class Lock>
class recursive_lock{
private:
  Lock _lock;
  std::thread::id _current_holder;
  size_t _permits;

  std::thread::id get_id(){
    return std::this_thread::get_id();
  }

  bool is_owner(){
    std::thread::id task_id = get_id();
    return _current_holder == task_id;
  }

  void set_owner(){
    _current_holder = get_id();
    _permits = 1;
  }

public:

  recursive_lock() : _lock(), _current_holder(0), _permits(0){
  }

  MARE_DELETE_METHOD(recursive_lock(recursive_lock&));
  MARE_DELETE_METHOD(recursive_lock(recursive_lock&&));
  MARE_DELETE_METHOD(recursive_lock& operator=(recursive_lock const&));
  MARE_DELETE_METHOD(recursive_lock& operator=(recursive_lock&&));

  void lock(){
    if(is_owner()){
      ++_permits;
    }else{
      _lock.lock();
      set_owner();
    }
  }

  bool try_lock(){
    if(is_owner()){
      ++_permits;
      return true;
    }else{
      bool success = _lock.try_lock();
      if(success){
        set_owner();
      }
      return success;
    }
  }

  void unlock(){
    --_permits;
    if(_permits == 0){
      _current_holder = std::thread::id(0);
      _lock.unlock();
    }
  }
};

/// Adds the capability to call try_lock for a specified
/// amount of time
template<class Lock>
class timed_lock{
private:
  Lock _lock;

public:
  static const size_t SPIN_THRESHOLD = 1000;

  timed_lock() : _lock(){
  }

  MARE_DELETE_METHOD(timed_lock(timed_lock&));
  MARE_DELETE_METHOD(timed_lock(timed_lock&&));
  MARE_DELETE_METHOD(timed_lock& operator=(timed_lock const&));
  MARE_DELETE_METHOD(timed_lock& operator=(timed_lock&&));

  void lock()
  {
    _lock.lock();
  }

  bool try_lock()
  {
    return _lock.try_lock();
  }

  template<class Rep, class Period>
  bool try_lock_for( const std::chrono::duration<Rep,Period>& timeout_duration)
  {
    auto end_time = std::chrono::high_resolution_clock::now() +
      timeout_duration;
    size_t local_spins = 0;

    while(_lock.try_lock() == false){
      if(end_time >= std::chrono::high_resolution_clock::now()){
        return false;
      }

      if(local_spins > SPIN_THRESHOLD){
        yield();
        local_spins = 0;
      }
    }

    return true;
  }

  template<class Clock, class Duration>
  bool try_lock_until(const std::chrono::
                      time_point<Clock,Duration>& timeout_time)
  {
    size_t local_spins = 0;

    while(_lock.try_lock() == false){
      if(timeout_time >= std::chrono::high_resolution_clock::now()){
        return false;
      }

      if(local_spins > SPIN_THRESHOLD){
        yield();
        local_spins = 0;
      }
    }

    return true;
  }
};

}; //namespace internal

}; //namespace mare
