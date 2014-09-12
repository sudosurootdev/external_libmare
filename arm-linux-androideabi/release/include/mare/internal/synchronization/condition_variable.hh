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

#include <atomic>
#include <memory>
#include <chrono>
#include <thread>

#include <mare/mutex.hh>

// Condition variable for MARE applications
//
// Currently implemented using a futex, which
// contains wait and wakeup methods that mirror
// the semantics of the condition variable
//
// The condition variable uses mare::mutex as the lock
// mechanism to avoid blocking to the OS.  mare::mutex
// may also yield to the scheduler if the mutex
// is not acquired quickly enough

namespace mare{

namespace internal{

class condition_variable {
private:
  futex _futex;

public:
  static const size_t SPIN_THRESHOLD = 1000;

  condition_variable() :
    _futex() { }

  ~condition_variable() { }

  MARE_DELETE_METHOD(condition_variable(condition_variable&));
  MARE_DELETE_METHOD(condition_variable(condition_variable&&));
  MARE_DELETE_METHOD(condition_variable& operator=(condition_variable const&));
  MARE_DELETE_METHOD(condition_variable& operator=(condition_variable&&));

  void wakeup() {
    notify_all();
  }

  void notify_one() {
    _futex.wakeup(1);
  }

  void notify_all() {
    _futex.wakeup(0);
  }

  void wait( std::unique_lock<mare::mutex>& lock) {
    lock.unlock();
    _futex.wait();
    lock.lock();
  }

  template< class Predicate >
  void wait( std::unique_lock<mare::mutex>& lock, Predicate pred) {
    while(!pred()){
      wait(lock);
    }
  }

  template< class Rep, class Period >
  mare::cv_status
  wait_for( std::unique_lock<mare::mutex>& lock,
            const std::chrono::duration<Rep,Period>& rel_time) {
    auto end_time = std::chrono::high_resolution_clock::now() + rel_time;
    return wait_until(lock,end_time);
  }

  template<class Rep, class Period, class Predicate>
  bool wait_for( std::unique_lock<mare::mutex>& lock,
                 const std::chrono::duration<Rep, Period>& rel_time,
                 Predicate pred) {
    while(!pred()){
      if(wait_for(lock, rel_time) == std::cv_status::timeout){
        return pred();
      }
    }
    return true;
  }

  template<class Clock, class Duration>
  mare::cv_status
  wait_until( std::unique_lock<mare::mutex>& lock,
              const std::chrono::time_point<Clock, Duration>& timeout_time) {
    lock.unlock();

    std::atomic<int>* state = _futex.timed_wait();

    while(std::chrono::high_resolution_clock::now() < timeout_time){
      if(*state == 1){
        break;
      }
      yield();
    }

    lock.lock();

    // Signal that we're done waiting
    int prev_state = state->exchange(1);
    if(prev_state == 0){
      // Timeout occurred before blocked_task in the futex was "woken"
      return std::cv_status::timeout;
    }

    // Successfully woken up before timeout. We have the last reference
    // to _state because the blocked_task has already been woken up
    delete state;
    return std::cv_status::no_timeout;
  }

  template<class Clock, class Duration, class Predicate>
  bool wait_until(std::unique_lock<mare::mutex>& lock,
                  const std::chrono::time_point<Clock, Duration>&
                  timeout_time,
                  Predicate pred) {
    while(!pred()){
      if(wait_until(lock, timeout_time) == std::cv_status::timeout){
        return pred();
      }
    }
    return true;
  }
};

template<typename Lock>
class condition_variable_any {

private:
  futex _futex;

public:
  static const size_t SPIN_THRESHOLD = 1000;

  condition_variable_any() :
    _futex() {}

  ~condition_variable_any() {}

  MARE_DELETE_METHOD(condition_variable_any(condition_variable_any&));
  MARE_DELETE_METHOD(condition_variable_any(condition_variable_any&&));
  MARE_DELETE_METHOD(condition_variable_any& operator=
                     (condition_variable_any const&));
  MARE_DELETE_METHOD(condition_variable_any& operator=
                     (condition_variable_any&&));

  void wakeup() {
    notify_all();
  }

  void notify_one() {
    _futex.wakeup(1);
  }

  void notify_all() {
    _futex.wakeup(0);
  }

  void wait( Lock& lock) {
    lock.unlock();

    _futex.wait();

    lock.lock();
  }

  template< class Predicate >
  void wait( Lock& lock, Predicate pred) {
    while(!pred()){
      wait(lock);
    }
  }

  template< class Rep, class Period >
  mare::cv_status
  wait_for( Lock& lock,
            const std::chrono::duration<Rep,Period>& rel_time) {
    auto end_time = std::chrono::high_resolution_clock::now() + rel_time;
    return wait_until(lock,end_time);
  }

  template<class Rep, class Period, class Predicate>
  bool wait_for( Lock& lock,
                 const std::chrono::duration<Rep, Period>& rel_time,
                 Predicate pred) {
    while(!pred()){
      if(wait_for(lock, rel_time) == std::cv_status::timeout){
        return pred();
      }
    }
    return true;
  }

  template<class Clock, class Duration>
  mare::cv_status
  wait_until( Lock& lock,
              const std::chrono::time_point<Clock, Duration>& timeout_time) {
    lock.unlock();

    std::atomic<int>* state = _futex.timed_wait();

    while(std::chrono::high_resolution_clock::now() < timeout_time){
      if(*state == 1){
        break;
      }
      yield();
    }

    lock.lock();

    // Signal that we're done waiting
    int prev_state = state->exchange(1);
    if(prev_state == 0){
      // Timeout occurred before blocked_task in the futex was "woken"
      return std::cv_status::timeout;
    }

    // Successfully woken up before timeout.  We have the last
    // reference to _state because the blocked_task has already been
    // woken up
    delete state;
    return std::cv_status::no_timeout;
  }

  template<class Clock, class Duration, class Predicate>
  bool wait_until( Lock& lock,
                   const std::chrono::
                   time_point<Clock, Duration>& timeout_time,
                   Predicate pred) {
    while(!pred()){
      if(wait_until(lock, timeout_time) == std::cv_status::timeout){
        return pred();
      }
    }
    return true;
  }
};

}; //namespace mare::internal
}; //namespace mare
