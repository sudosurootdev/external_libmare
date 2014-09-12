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
/** @file condition_variable.hh */
#pragma once

#include <mare/internal/synchronization/condition_variable.hh>


namespace mare{

/** @cond */
template<class Lock>

struct condition_variable_any{
  typedef internal::condition_variable_any<Lock> type;
};
/** @endcond */

typedef internal::condition_variable condition_variable;

}; //namespace mare

#ifdef ONLY_FOR_DOXYGEN

#include <mutex>

#include <mare/mutex.hh>

namespace mare{

/** @addtogroup sync
@{ */
/**
    Provides inter-task communication via wait() and notify().

    Provides inter-task communication via wait() and notify() calls
    without blocking the MARE scheduler.
*/
class condition_variable {
public:

  /** Default constructor. */
  constexpr condition_variable();

  condition_variable(condition_variable&) = delete;
  condition_variable(condition_variable&&) = delete;
  condition_variable& operator=(condition_variable const&) = delete;

  /**  Wakes up one waiting task. */
  void notify_one();

  /**  Wakes up all waiting tasks. */
  void notify_all();

  /** Task waits until woken up by another task.

      Task may yield to the MARE scheduler.

      @param lock Mutex associated with the condition_variable.
      Must be locked prior to calling wait. Mutex will be
      held by the task when wait returns.
  */
  void wait(std::unique_lock<mare::mutex>& lock);

  /** While the predicate is false, the task waits.

      Task may yield to the MARE scheduler.

      @param lock Mutex associated with the condition_variable.
      Must be locked prior to calling wait. lock will be
      held by the task when wait returns.

      @param pred The condition_variable will continue to wait
      until this Predicate is true.
  */
  template< class Predicate >
  void wait( std::unique_lock<mare::mutex> & lock, Predicate pred);

  /** Task waits until either 1) woken by another task or
      2) wait time has been exceeded.

      Task may yield to the MARE scheduler.

      @param lock Mutex associated with the condition_variable.
      Mutex must be locked prior to calling wait_for.

      @param rel_time Amount of time to wait to be woken up.

      @return
      std::cv_status::no_timeout - Task was woken up by another task
      before rel_time was exceeded.  lock will be held by the task when
      wait_for returns.
      @par
      std:cv_status::timeout - rel_time was exceeded before the
      task was woken up.
  */
  template< class Rep, class Period >
  std::cv_status wait_for( std::unique_lock<mare::mutex> & lock,
                           const std::chrono::duration<Rep,Period> & rel_time);

  /** While the predicate is false, task waits until 1) woken by
      another task or 2) wait time has been exceeded.

      Task may yield to the MARE scheduler.

      @param lock Mutex associated with the condition_variable.
      Mutex must be locked prior to calling wait_for.

      @param rel_time Amount of time to wait to be woken up.

      @param pred The condition_variable will continue to wait
      until this Predicate is true.

      @return
      TRUE -- Task was woken up and predicate was true before rel_time
      was exceeded. lock will be held by the task when wait_for returns.
      @par
      FALSE -- Predicate was not true before rel_time was exceeded.
  */
  template<class Rep, class Period, class Predicate>
  bool wait_for( std::unique_lock<mare::mutex> & lock,
                 const std::chrono::duration<Rep, Period> & rel_time,
                 Predicate pred);

  /** Task waits until either 1) woken by another task or
      2) wait time has been exceeded.

      Task may yield to the MARE scheduler.

      @param lock Mutex associated with the condition_variable.
      Mutex must be locked prior to calling wait_until.

      @param timeout_time Time to wait until.

      @return
      std::cv_status::no_timeout -- Task was woken up by another task
      before timeout_time was exceeded. lock will be held by the task when
      wait_until returns.
      @par
      std:cv_status::timeout -- timeout_time was exceeded before the
      task was woken up.
  */
  template<class Clock, class Duration>
  std::cv_status
  wait_until(std::unique_lock<mare::mutex>& lock,
             const std::chrono::time_point<Clock, Duration>& timeout_time);

  /** While the predicate is false, task waits until 1) woken by
      another task or 2) wait time has been exceeded.

      Task may yield to the MARE scheduler.

      @param lock Mutex associated with the condition_variable.
      Mutex must be locked prior to calling wait_until.

      @param timeout_time Amount of time to wait to be woken up.

      @param pred The condition_variable will continue to wait
      until this Predicate is true.

      @return
      TRUE -- Task was woken up and predicate was true before timeout_time
      was exceeded. lock will be held by the task when wait_until returns.
      @par
      FALSE -- Predicate was not true before timeout_time was exceeded.
  */
  template<class Clock, class Duration, class Predicate>
  bool wait_until(std::unique_lock<mare::mutex>& lock,
                  const std::chrono::time_point<Clock, Duration>& timeout_time,
                  Predicate pred);
};
/** @} */ /* end_addtogroup sync */
};


#endif //ONLY_FOR_DOXYGEN
