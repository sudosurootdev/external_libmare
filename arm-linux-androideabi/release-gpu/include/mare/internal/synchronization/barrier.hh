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

#include <mare/task.hh>
#include <mare/common.hh>
#include <mare/internal/synchronization/mutex.hh>

#include <cstdlib>
#include <atomic>

// Contains barrier interface for MARE applications
//
// Currently implemented as a sense reversing spin-barrier
// _count is atomically decremented by each task that waits on the
// barrier.  When _count reaches 0, the task that last decremented
// _count negates _sense to allow the waiting tasks to continue
// execution.
//
// When a task exceeds the SPIN_THRESHOLD, all further waiting
// tasks will yield() to the MARE scheduler
//
// For responsiveness, some amount of spinning is done before
// yielding to the MARE scheduler

namespace mare{

namespace internal{

class sense_barrier{
private:
  // Counts the number of tasks that still need to reach the barrier
  std::atomic<size_t> _count;
  // Total number of tasks that will wait on the barrier
  // Does not change after initialization
  size_t _total;
  // Current direction of the barrier
  bool _sense;
  // Tasks (for each sense) to wait on if too much spinning occurs
  mare::task_ptr _wait_task[2];

  size_t _delta;

public:
  static const int WAIT_TASK_SPIN_THRESHOLD = 1000;
  static const int SPIN_THRESHOLD = 10000;

  void wait();

  sense_barrier(size_t count) : _count(count), _total(count),
                                _sense(false), _wait_task(),
                                _delta(_total / (4))
  {
    _wait_task[0] = nullptr;
    _wait_task[1] = nullptr;
  }

  MARE_DELETE_METHOD(sense_barrier(sense_barrier&));
  MARE_DELETE_METHOD(sense_barrier(sense_barrier&&));
  MARE_DELETE_METHOD(sense_barrier& operator=(sense_barrier const&));
  MARE_DELETE_METHOD(sense_barrier& operator=(sense_barrier&&));

private:

  bool volatile_read_sense(){
    return *(static_cast<volatile bool*>(&_sense));
  }

  void create_wait_task(bool local_sense);
};

}; //namespace internal

}; //namespace mare
