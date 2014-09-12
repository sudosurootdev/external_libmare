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

#include <array>
#include <atomic>
#include <map>

#include <mare/internal/debug.hh>
#include <mare/internal/log/loggerbase.hh>
#include <mare/internal/log/objectid.hh>

namespace mare {
namespace internal {
namespace log {

/**
   Logs MARE events with ftrace using Qualcomm
   testframework format
*/
class ftrace_logger : public logger_base {

public:

#ifndef MARE_USE_FTRACE_LOGGER
  typedef std::false_type enabled;
#else
  typedef std::true_type enabled;
#endif

  // Initializes ftrace_logger data structures
  static void init();

  // Destroys ftrace_logger data structures
  static void shutdown();

  // Outputs buffer contents
  static void dump() { }

  // Pauses the logging infrastructure
  static void paused();

  // Resumes the logging infrastructure
  static void resumed();

  // Logs event. Because we want events as inlined as possible, the
  // implementation for the method is in ftracelogger.hh
  template<typename EVENT>
  static void log(EVENT&& e, event_context&) {
    print(std::forward<EVENT>(e));
  }

private:

  static void ftrace_write(const char *str);
  static void ftrace_write(const char *str, size_t len);

  static void print(events::group_canceled&& e);

  static void print(events::group_created&& e);

  static void print(events::group_destroyed&& e);

  static void print(events::group_reffed&& e);

  static void print(events::group_unreffed&& e);

  static void print(events::task_after&& e);

  static void print(events::task_cleanup&& e);

  static void print(events::task_created&& e);

  static void print(events::task_destroyed&& e);

  static void print(events::task_done&& e);

  static void print(events::task_executes&& e);

  static void print(events::task_sent_to_runtime&& e);

  static void print(events::task_reffed&& e);

  static void print(events::task_unreffed&& e);

  static void print(events::task_wait&& e);

  template<typename UNKNOWNEVENT>
  static void print(UNKNOWNEVENT&&) { }

  MARE_DELETE_METHOD(ftrace_logger());
  MARE_DELETE_METHOD(ftrace_logger(ftrace_logger const&));
  MARE_DELETE_METHOD(ftrace_logger(ftrace_logger&&));
  MARE_DELETE_METHOD(ftrace_logger&
                     operator=(ftrace_logger const&));
  MARE_DELETE_METHOD(ftrace_logger& operator=(ftrace_logger&&));


}; // class ftrace_logger

} // namespace mare::internal::log
} // namespace mare::internal
} // namespace mare
