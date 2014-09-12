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
#include <condition_variable>
#include <chrono>

#include <mare/exceptions.hh>
#include <mare/common.hh>

namespace mare {

namespace internal {

class semaphore {
public:
  semaphore(bool flag = false)
    : _flag(flag),
      _mutex(),
      _cv()
  {}

  ~semaphore() {}

  MARE_DELETE_METHOD(semaphore(semaphore const&));
  MARE_DELETE_METHOD(const semaphore& operator=(semaphore const&));

  void signal() {
    reset(true);
  }

  template<typename Fn>
  void signal(Fn&& fn) {
    std::lock_guard<std::mutex> lock(_mutex);
    fn();
    _flag = true;
    _cv.notify_one();
  }

  void reset(bool flag = false) {
    std::lock_guard<std::mutex> lock(_mutex);
    _flag = flag;
    if (flag)
      _cv.notify_one();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(_mutex);
    while (!_flag)
      _cv.wait(lock);
    _flag = false;
  }

  template<class Rep, class Period>
  mare::cv_status
  wait_for(std::chrono::duration<Rep, Period> const& rel_time) {
    std::unique_lock<std::mutex> lock(_mutex);
    while (!_flag) {
      if (_cv.wait_for(lock, rel_time) == std::cv_status::timeout)
        return std::cv_status::timeout;
    }
    _flag = false;
    return std::cv_status::no_timeout;
  }

  std::mutex& mutex() {
    return _mutex;
  }

protected:
  bool _flag;
  std::mutex _mutex;
  std::condition_variable _cv;
};

class counting_semaphore {
public:
  counting_semaphore(size_t count = 1)
    : _count(count),
      _mutex(),
      _cv()
  {}

  ~counting_semaphore() {}

  MARE_DELETE_METHOD(counting_semaphore(counting_semaphore const&));
  MARE_DELETE_METHOD(const counting_semaphore& operator=
                     (counting_semaphore const&));

  void signal(size_t times = 1) {
    std::lock_guard<std::mutex> lock(_mutex);
    MARE_API_ASSERT(_count >= times, "unbalanced signals");
    size_t res = (_count -= times);
    if (res == 0) {
      _cv.notify_all();
    }
  }

  void wait() {
    std::unique_lock<std::mutex> lock(_mutex);
    while (_count > 0)
      _cv.wait(lock);
  }

  template<class Rep, class Period>
  mare::cv_status
  wait_for(std::chrono::duration<Rep, Period> const& rel_time) {
    std::unique_lock<std::mutex> lock(_mutex);
    while (_count > 0) {
      if (_cv.wait_for(lock, rel_time) == std::cv_status::timeout)
        return std::cv_status::timeout;
    }
    return std::cv_status::no_timeout;
  }

  void reset(size_t count = 1) {
    std::lock_guard<std::mutex> lock(_mutex);
    _count = count;
  }

protected:
  size_t _count;
  std::mutex _mutex;
  std::condition_variable _cv;
};

}; // namespace internal

}; // namespace mare
