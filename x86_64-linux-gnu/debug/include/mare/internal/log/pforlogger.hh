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
    Record events relevant to work steal tree.
*/
class pfor_logger : public logger_base {

public:

  static const auto relaxed = std::memory_order_relaxed;

#ifndef MARE_USE_PFOR_LOGGER
  typedef std::false_type enabled;
#else
  typedef std::true_type enabled;
#endif

  // Initializes pfor_logger data structures
  static void init();

  // Destroys pfor_logger data structures
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

  static void count(events::ws_tree_new_slab&&) {
    s_num_slabs.fetch_add(1, relaxed);
  }

  static void count(events::ws_tree_node_created&&) {
    s_num_nodes.fetch_add(1, relaxed);
  }

  static void count(events::ws_tree_worker_try_own&&) {
    s_num_own.fetch_add(1, relaxed);
  }

  static void count(events::ws_tree_worker_try_steal&&) {
    s_num_steal.fetch_add(1, relaxed);
  }

  static void count(events::ws_tree_try_own_success&&) {
    s_success_own.fetch_add(1, relaxed);
  }

  static void count(events::ws_tree_try_steal_success&&) {
    s_success_steal.fetch_add(1, relaxed);
  }

  template<typename UnknownEvent>
  static void count(UnknownEvent&&) { }

  static std::atomic<size_t> s_num_nodes;
  static std::atomic<size_t> s_num_own;
  static std::atomic<size_t> s_num_slabs;
  static std::atomic<size_t> s_num_steal;
  static std::atomic<size_t> s_success_own;
  static std::atomic<size_t> s_success_steal;

  MARE_DELETE_METHOD(pfor_logger());
  MARE_DELETE_METHOD(pfor_logger(pfor_logger const&));
  MARE_DELETE_METHOD(pfor_logger(pfor_logger&&));
  MARE_DELETE_METHOD(pfor_logger& operator=(pfor_logger const&));
  MARE_DELETE_METHOD(pfor_logger& operator=(pfor_logger&&));

}; // class pfor_logger

} // namespace mare::internal::log
} // namespace mare::internal
} // namespace mare
