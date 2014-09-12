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

namespace mare
{

namespace internal
{

class thread_manager_condvar {
public:
  thread_manager_condvar(size_t qs) :
    _gen(),
    _waiters(),
    _qs(qs),
    _last(qs),
    _mutex(),
    _condvar() {}
  ~thread_manager_condvar() {}

  template <typename PREDICATE>
  void idle(PREDICATE pred, size_t qidx) {
    qidx = 0;
    std::unique_lock<std::mutex> lock(_mutex);
    if (pred()) {
      if (_last[qidx] == _gen) {
        ++_waiters;
        _condvar.wait(lock);
        --_waiters;
      }
      _last[qidx] = _gen;
    }
  }

  void notify_all() {
    std::unique_lock<std::mutex> lock(_mutex);
    _condvar.notify_all();
  }

  void notify(size_t qidx, size_t nelem, bool fuzzy) {
    (void)qidx;

    size_t check = 2;
    if (fuzzy)
      check = _qs;

    if (nelem < check) {
      std::unique_lock<std::mutex> lock(_mutex);
      ++_gen;
      if (_waiters == 0)
        return;
    }
    _condvar.notify_all();
  }

private:
  size_t _gen;
  size_t _waiters;
  size_t _qs;
  std::vector<size_t> _last;
  std::mutex _mutex;
  std::condition_variable _condvar;
};

} //namespace internal

} //namespace mare
