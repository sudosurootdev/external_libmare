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
/** @file powermanager.hh */
#pragma once

#include <chrono>
// #include <mare/common.hh>

namespace mare {

namespace power {

/** @addtogroup static_power_doc
    @{ */

enum class mode {
  normal, /**< Device default power settings */
  efficient, /**< Enable all available cores and reduce the maximum frequency.
               Useful for parallel applications with work bursts */
  perf_burst, /**< Enable all cores at the maximum frequency for a limited
                   amount of time in the order of seconds. Actual time will
                   depends on the device, the rest of running applications and
                   the environment */
  saver, /**< Enable half total cores with maximum frequency limited to the
           median of the available frequencies */
};
/**
  Set the system in one of the predefined performance/power modes

  Depending on the load and the system status, requests can be ignored.
  For example, in case of thermal throttling. The programmer can specify
  the duration of the mode in milliseconds or can pass a 0 in the duration
  parameter. In this case, the mode remains active until: the application
  finishes, a call to set_static_mode(mode::normal), a call to
  set_goal()

  @param m performance/power mode. Possible values: NONE, efficient,
  perf_burst, and saver
  @param duration Time in milliseconds that the mode should be enabled.
  perf_burst mode has a limited maximum duration depending on the device
  and environment. If duration is lower than 0, the function returns
  immediately.

  @par Example 1 Setting the efficent mode.
  @includelineno snippets/power_static.cc
*/
void request_mode(const mode m, const std::chrono::milliseconds& duration =
        std::chrono::milliseconds(0));

/** @} */ /* end_addtogroup static_power_doc */

/** @addtogroup dynamic_power_doc
    @{ */
/**
  Start the automatic performance/power regulation mode.

  Once the regulation process starts, subsequent calls to this function
  are silently ignored.

  @param desired User defined performance metric value to stabilize the
  runtime. The metric unit has to match with the unit of the measured
  parameter of regulate. For better results please use values ranging
  between ten and hundred.
  @param tolerance Percentage of allowed error between the desired and
  the measured values. If the error is less than the tolerance, the system
  remains in the same state. For example, in the quotient between measured
  and desired is 0.95 and tolerance is 0.1, state will not change.

  When the tolerance is met, the system will remain
  in the current state. If tolerance is 0.0, the runtime tries to adjust
  the system at every step
*/
void set_goal(const float desired, const float tolerance = 0.0);

/**
  Terminate the regulation process and return the system to normal mode
*/
void clear_goal();

/**
  Regulate the system to achieve the desired performance level

  @param measured Performance metric value observed during the last
  iteration

  @par Example 2 Dynamic regulation.
  @includelineno snippets/power_dynamic.cc
*/
void regulate(const float measured);

} // namespace power
} // namespace mare

/** @} */ /* end_addtogroup dynamic_power_doc */

