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

namespace mare { namespace internal {

/// Records the state of a task
///
/// UNLAUNCHED: Task in UNLAUNCHED state cannot execute, even if they have
///             no predecessors.
///
/// READY: Tasks in READY state are ready to execute as soon as the number of
///        predecessors is 0.
///
/// RUNNING: Task state once MARE's scheduler starts executing a
///          task. If a task is in this state, it doesn't mean that it
///          is actually running on a device.  For example, a task
///          state can be RUNNING but the task might be blocked
///          waiting for another task to complete.
///
/// COMPLETED: Task state after it successfully finishes execution.
///
/// CANCELED: A task t will eventually transition to CANCELED if:
///            - t->cancel() was called before the task started running.
///            - t->cancel() was called while the task was running and
///              the task acknowledges that it saw the cancelation.
///            - The task throws an exception during execution and
///               the programmer didn't catch it.
///
/// The following diagram depicts the all the possible state transitions
/// for a task. Notice that there are no back arrows.
///
///
///
/// UNLAUNCHED--> READY --> RUNNING ~~> COMPLETED
///                  |
///                   ~~~~> CANCELED
///
/// Transitions depicted by ~~> grab the task' lock.
///
/// State are represented with an uint_fast8_t type. Their value
/// is important because sometimes we check that the state is
/// <= that RUNNING.
///
///
class task_state{
public:
  typedef uint32_t type;
  static const type CANCELED   = 1lu << ((sizeof(type) * 8) - 1);
  static const type COMPLETED  = CANCELED   >> 1;
  static const type RUNNING    = COMPLETED  >> 1;
  static const type UNLAUNCHED = RUNNING    >> 1;
  static const type CANCEL_REQ = UNLAUNCHED >> 1;
  static const type IN_UTCACHE = CANCEL_REQ >> 1;

  static const type CANCELED_MASK   = ~CANCELED;
  static const type COMPLETED_MASK  = ~COMPLETED;
  static const type RUNNING_MASK    = ~RUNNING;
  static const type UNLAUNCHED_MASK = ~UNLAUNCHED;
  static const type CANCEL_REQ_MASK = ~CANCEL_REQ;
  static const type IN_UTCACHE_MASK = ~IN_UTCACHE;

  static const type PRED_MASK = ~(CANCELED |
                                  COMPLETED |
                                  RUNNING |
                                  UNLAUNCHED |
                                  CANCEL_REQ |
                                  IN_UTCACHE);
  static const type MAX_PREDS = PRED_MASK  - 1;

  constexpr task_state():_state(0){}
  constexpr task_state(type state):_state(state){}

  constexpr type get_raw_value() const {
    return _state;
  }

  task_state& operator=(task_state const& other) {
    if (this != &other)
      _state = other._state;
    return *this;
  }

  ///Checks whether the task finished execution.
  ///
  ///@return Returns true if the task transitioned to COMPLETED or
  ///CANCELED, regardless of whether the task has been marked for
  ///cancelation or not.
  constexpr bool is_done() const {
    return (_state >= COMPLETED);
  }

  ///Checks whether the task successfully finished execution.
  ///
  ///@return Returns true if the task transitioned to COMPLETED
  ///regardless of whether the task has been marked for cancelation
  ///or not.
  constexpr bool is_completed() const {
    return ((_state & COMPLETED) !=0);
  }

  /// Returns the number of task that must complete, cancel, or
  /// except before the task can be executed.
  ///
  /// @return Number of predecessors for a task.
  constexpr size_t get_num_predecessors() const {
    return static_cast<size_t>(_state & PRED_MASK);
  }

  ///Checks whether the task is executing.
  ///
  ///@return Returns true is in RUNNING state, regardless of whether
  ///the task has been marked for cancelation or not.
  constexpr bool is_running() const {
    return ((_state & RUNNING) != 0);
  }

  ///Checks whether the task can be placed in the ready queue.
  ///
  ///@return Returns true if the task is launched, has no
  ///predecessors and it hasn't been marked for cancelation.
  constexpr bool is_ready() const {
    return (_state == 0 || _state == IN_UTCACHE);
  }

  ///Checks whether the task has been launched or not.
  ///
  ///@return Returns true if the task hasn't been launched,
  ///regardless of whether the task has been marked for cancelation
  ///or not.
  constexpr bool is_launched() const {
    return ((_state & UNLAUNCHED) == 0);
  }

  ///Checks whether the task has been marked for cancelation
  ///
  ///@return Returns true if the task has been marked for cancelation,
  /// irrespective of which state it is
  constexpr bool is_marked_for_cancelation() const {
    return((_state & CANCEL_REQ) != 0);
  }

  /// Checks whether the task is in the unlaunched task cache
  ///
  /// @return
  /// true  - Returns true if the task is in the unlaunched task cache.\n
  /// false - Returns false if the task is not in the unlaunched task cache.
  constexpr bool in_utcache() const {
    return((_state & IN_UTCACHE) != 0);
  }

  ///Checks whether the task has started running or not.
  ///
  ///@return Returns true if the task is not running yet, regardless of
  ///whether the task has been marked for cancelation.
  constexpr bool is_not_running_yet() const {
    return _state < RUNNING;
  }

  ///Checks whether the task has transitioned to the CANCELED state
  ///
  ///@return Returns true if the task is in CANCELED state, regardless of
  ///whether the task has been marked for cancelation.
  constexpr bool is_canceled() const {
    return _state >= CANCELED;
  }

private:
  type _state;
}; //class task_state

} //namespace internal
} //namespace mare

