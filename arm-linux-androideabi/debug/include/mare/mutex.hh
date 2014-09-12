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
/** @file mutex.hh */
#pragma once

#include <chrono>

#include <mare/internal/synchronization/mutex.hh>

namespace mare{
/** @cond */
typedef internal::mutex mutex;
typedef internal::timed_lock<mare::mutex> timed_mutex;
typedef internal::recursive_lock<mare::mutex> recursive_mutex;
typedef internal::timed_lock<mare::recursive_mutex> recursive_timed_mutex;

typedef internal::futex futex;
/** @endcond */
}; //namespace mare

#ifdef ONLY_FOR_DOXYGEN

namespace mare {

/** @addtogroup sync
@{ */
/**
    Provides basic mutual exclusion.

    Provides exclusive access to a single task or thread without
    blocking the MARE scheduler.
*/
class mutex{
public:

  /** Default constructor. Initializes to not locked. */
  constexpr mutex();

  mutex(mutex&) = delete;
  mutex(mutex&&) = delete;
  mutex& operator=(mutex const&) = delete;

  /** Destructor. */
  ~mutex();

  /**
      Acquires the mutex, waits if the mutex is not available.

      May yield to the MARE scheduler.
  */
  void lock();

  /**
      Attempts to acquire the mutex.

      @return
      TRUE -- Successfully acquired the mutex.\n
      FALSE -- Did not acquire the mutex.
  */
  bool try_lock();

  /** Releases the mutex. */
  void unlock();
};

/**
    Provides wait/wakeup capabilities for implementing
    synchronization primitives.

    Allows a task to wait for another task to explicitly wake it
    up. Does not block to the operating system.
*/
class futex{
public:

  /** Default constructor. */
  futex();

  /** Destructor. */
  ~futex();

  futex(futex&) = delete;
  futex(futex&&) = delete;
  futex& operator=(futex const&) = delete;

  /**
      Current task will block to the MARE scheduler until
      another task calls wakeup.
  */
  void wait();

  /**
      Attempts to wake up requested number of waiting tasks.

      @param num_tasks Number of tasks to attempt to wake up. Will
      only wake up tasks if enough exist to be woken up.
      num_tasks == 0 is reserved for broadcast (i.e., will wake up
      as many tasks as possible).

      @return Number of tasks that were successfully woken up.
  */
  size_t wakeup(size_t num_tasks);
};
/** @} */ /* end_addtogroup sync */
};


#endif //ONLY_FOR_DOXYGEN
