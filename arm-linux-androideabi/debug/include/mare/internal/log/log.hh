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

#include <mare/internal/log/eventcounterlogger.hh>
// Enable the ftrace logger only for Android and Linux
#if defined(__ANDROID__) || defined(__linux__)
#include <mare/internal/log/ftracelogger.hh>
#endif
#include <mare/internal/log/infrastructure.hh>
#include <mare/internal/log/imlogger.hh>
#include <mare/internal/log/pforlogger.hh>

namespace mare {
namespace internal {
namespace log {

//List of supported loggers. Please keep imlogger first because we
//want it to be the first one.
//ToDo: Transition to a typelist
#if defined(__ANDROID__) || defined(__linux__)
typedef infrastructure<imlogger, event_counter_logger, ftrace_logger,
                       pfor_logger> loggers;
#else
typedef infrastructure<imlogger, event_counter_logger, pfor_logger> loggers;
#endif


typedef loggers::group_id group_id;
typedef loggers::task_id task_id;

void init();
void shutdown();

void pause();
void resume();
void dump();


// Logs event e.
template<typename EVENT>
inline void log_event(EVENT&& e) {
  loggers::event(std::forward<EVENT>(e));
}

} // mare::internal::log
} // mare::internal
} // mare
