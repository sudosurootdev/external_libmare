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
/** @file barrier.hh */
#pragma once

#include <mare/internal/synchronization/barrier.hh>

namespace mare{
/** @cond */
typedef internal::sense_barrier barrier;
/** @endcond */

}; //namespace mare

#ifdef ONLY_FOR_DOXYGEN

namespace mare {

/** @addtogroup sync
@{ */
/**
    Synchronizes multiple tasks at the call to wait().

    Synchronizes multiple tasks (or threads) at the point where
    wait() is called without blocking the MARE scheduler.
*/
class barrier{
public:

  /**
      Constructs the barrier, given the number of tasks
      that will wait on it.

      @param total Number of tasks that will wait on the barrier.
  */
  barrier(size_t total);

  barrier(barrier&) = delete;
  barrier(barrier&&) = delete;
  barrier& operator=(barrier const&) = delete;
  barrier& operator=(barrier&&) = delete;

  /**
      Wait on the barrier.

      Task may yield to the MARE scheduler.
  */
  void wait();
};
/** @} */ /* end_addtogroup sync */
};


#endif //ONLY_FOR_DOXYGEN
