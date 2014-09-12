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
   Counts MARE events.
*/
class event_counter_logger : public logger_base {

public:

#ifndef MARE_USE_EVENT_COUNTER_LOGGER
  typedef std::false_type enabled;
#else
  typedef std::true_type enabled;
#endif

  // Initializes event_counter_logger data structures
  static void init();

  // Destroys event_counter_logger data structures
  static void shutdown();

  // Outputs buffer contents
  static void dump();

  // Logging infrastructure has been paused
  static void paused(){};

  // Logging infrastructure has been resumed
  static void resumed(){};

  // Logs event. Because we want events as inlined as possible, the
  // implementation for the method is in event_counter_loggerevents.hh
  template<typename Event>
  static void log(Event&& e, event_context&) {
    count(std::forward<Event>(e));
  }


private:

  static void count(events::task_created&&) {
    s_num_tasks.fetch_add(1, std::memory_order_relaxed);
  }

  static void count(events::group_created&&) {
    s_num_groups.fetch_add(1, std::memory_order_relaxed);
  }


  template<typename UnknownEvent>
  static void count(UnknownEvent&&) { }

  static std::atomic<size_t> s_num_tasks;
  static std::atomic<size_t> s_num_groups;

  MARE_DELETE_METHOD(event_counter_logger());
  MARE_DELETE_METHOD(event_counter_logger(event_counter_logger const&));
  MARE_DELETE_METHOD(event_counter_logger(event_counter_logger&&));
  MARE_DELETE_METHOD(event_counter_logger&
                     operator=(event_counter_logger const&));
  MARE_DELETE_METHOD(event_counter_logger& operator=(event_counter_logger&&));

}; // class event_counter_logger

} // namespace mare::internal::log
} // namespace mare::internal
} // namespace mare
