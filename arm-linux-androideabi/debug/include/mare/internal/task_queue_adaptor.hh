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
#include <condition_variable>
#include <mutex>
#include <deque>

#include <mare/internal/dualtaskqueue.hh>
#include <mare/internal/chaselevdeque.hh>
#include <mare/internal/mutex_cv.hh>

namespace mare
{

namespace internal
{

class task;

template <class _Key> class task_queue_adaptor { };

enum class task_queue_notify {
  NONE, ONE, ALL
};

template <>
class task_queue_adaptor<std::deque<task*>> {
 public:
  static constexpr bool pop_is_mt_safe() { return true; }
  static constexpr bool element_count_is_fuzzy() { return false; }

  task_queue_adaptor<std::deque<task*>>() :
    _q(),
    _mutex(),
    _condvar() {}

  void threadInitAuto() {}   // for backward comaptibility
  size_t unsafe_size() { return _q.size(); }
  size_t steal(task *& t, paired_mutex_cv* synch);
  void notify_all(paired_mutex_cv&) { _condvar.notify_all(); }

  size_t push(task* t, task_queue_notify tqn);
  size_t pop(task *& t, paired_mutex_cv*);
  template <typename PREDICATE>
    size_t pop_if(PREDICATE pred, task*& t, paired_mutex_cv*,
                  task_queue_notify tqn = task_queue_notify::ALL);

 private:
  std::deque<task*> _q;
  std::mutex _mutex;
  std::condition_variable _condvar;
};

template <>
class task_queue_adaptor<DualTaskQueue<task*>> {
 public:
  static constexpr bool pop_is_mt_safe() { return true; }
  static constexpr bool element_count_is_fuzzy() { return true; }

  task_queue_adaptor<DualTaskQueue<task*>>() :
    _q(),
    _counter(0) {}

  void threadInitAuto() { _q.threadInitAuto(); }
  size_t unsafe_size() const { return _counter.load(); }

  size_t push(task* t, task_queue_notify tqn = task_queue_notify::ALL) {
    // we need to move this to every pthread start.
    _q.threadInitAuto();

    // make sure we add before we push
    auto c = _counter.fetch_add(1);
    _q.push(t, tqn != task_queue_notify::NONE);

    return c + 1;
  }

  size_t pop(task *& t, paired_mutex_cv* synch) {
    ssize_t c = 0;

    if (_q.popIf([] () -> bool { return false; }, t, synch)) {
      c = _counter.fetch_sub(1);
    }
    return c;
  }

  size_t steal(task *&t, paired_mutex_cv* synch) {
    _q.threadInitAuto();
    return pop(t, synch);
  }

  template <typename PREDICATE>
    size_t pop_if(PREDICATE pred, task*& t, paired_mutex_cv* synch,
                  task_queue_notify tqn = task_queue_notify::ALL) {
    if (tqn == task_queue_notify::NONE)
      return pop(t, synch);

    ssize_t c = 0;
    if (_q.popIf(pred, t, synch)) {
      c = _counter.fetch_sub(1);
    }
    return c;
  }

  void notify_all(paired_mutex_cv& synch) {
    std::unique_lock<std::mutex> lock(synch.first);
    synch.second.notify_one();
  }

 private:
  DualTaskQueue<task*> _q;
  std::atomic<ssize_t> _counter;
};

//
// std::deque
//

inline size_t
task_queue_adaptor<std::deque<task*>>::push(
    task* t, task_queue_notify tqn = task_queue_notify::ALL)
{
  std::lock_guard<std::mutex> lock(_mutex);
  _q.push_back(t);

  switch (tqn) {
  default:
  case task_queue_notify::ALL:
    _condvar.notify_all();
    break;
  case task_queue_notify::ONE:
    _condvar.notify_one();
    break;
  case task_queue_notify::NONE:
    break;
  }
  return _q.size();
}

inline size_t
task_queue_adaptor<std::deque<task*>>::pop(task *& t, paired_mutex_cv*)
{
  std::lock_guard<std::mutex> lock(_mutex);
  if (_q.empty())
    return 0;
  size_t sz = _q.size();
  t = _q.back();
  _q.pop_back();
  return sz;
}

inline size_t
task_queue_adaptor<std::deque<task*>>::steal(task *& t, paired_mutex_cv*)
{
  std::lock_guard<std::mutex> lock(_mutex);
  if (_q.empty())
    return 0;
  size_t sz = _q.size();
  t = _q.front();
  _q.pop_front();
  return sz;
}

template <typename PREDICATE>
size_t
task_queue_adaptor<std::deque<task*>>::pop_if(PREDICATE pred, task*& t,
                                              paired_mutex_cv*,
                                              task_queue_notify tqn)
{
  std::unique_lock<std::mutex> lock(_mutex);

  do {
    if (pred() && _q.empty()) {
      if (tqn != task_queue_notify::NONE)
        _condvar.wait(lock);
    } else if (_q.empty()) {
      return 0;
    } else {
      t = _q.back();
      size_t sz = _q.size();
      _q.pop_back();
      return sz;
    }
  } while (tqn != task_queue_notify::NONE);
  return 0;
}

template <>
class task_queue_adaptor<chase_lev_deque<task*>> {
public:
  static constexpr bool pop_is_mt_safe() { return false; }
  static constexpr bool element_count_is_fuzzy() { return false; }

  task_queue_adaptor<chase_lev_deque<task*>>() : _q() {}

  size_t unsafe_size() { return _q.unsafe_size(); }

  size_t push(task* t, task_queue_notify tqn = task_queue_notify::NONE) {
    MARE_INTERNAL_ASSERT(tqn == task_queue_notify::NONE, "no notify support");
    return _q.push(t);
  }

  // for backward comaptibility
  void threadInitAuto() { }
  size_t pop(task *& t, paired_mutex_cv*) { return _q.take(t); }

  size_t steal(task*& t, paired_mutex_cv*) {
    auto sz = _q.steal(t);

    // if we got ABORT, that means we lost the race, and the queue
    // we are trying to steal from is "shallow". For now we just
    // treat it like EMPTY
    if (sz == _q.ABORT)
      return 0;
    return sz;
  }

private:
  chase_lev_deque<task*> _q;
};

typedef task_queue_adaptor<DualTaskQueue<task*>> local_dual_task_queue;
typedef task_queue_adaptor<std::deque<task*>> local_std_task_queue;
typedef task_queue_adaptor<chase_lev_deque<task*>> local_cld_task_queue;

} //namespace internal

} //namespace mare
