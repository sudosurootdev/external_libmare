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

namespace mare {
namespace internal {
namespace log {

/**
   Logs the lat s_size MARE events in an in-memory buffer.
   imlogger tries to be as fast as possible.
*/
class imlogger : public logger_base {

  // Circular buffer size. Type and constant.
  typedef event_context::counter_type index_type;
  static constexpr index_type s_size = 512;

public:

#ifndef MARE_USE_IMLOGGER
  typedef std::false_type enabled;
#else
  typedef std::true_type enabled;
#endif

  // Initializes imlogger data structures
  static void init();

  // Destroys imlogger data structures
  static void shutdown();

  // Outputs buffer contents
  static void dump();

  // Logging infrastructure has been paused
  static void paused(){};

  // Logging infrastructure has been resumed
  static void resumed(){};


  // Logs event. Because we want events as inlined as possible, the
  // implementation for the method is in imloggerevents.hh
  template<typename Event>
  static void log(Event&& e, event_context& context);

  // log_entry represents an entry in the circular buffer
  // Each log_entry stores:
  //  - event counter
  //  - event id
  //  - id of the thread that executed the event
  //  - buffer for event handler

  struct entry {

    // Maximum payload size. Size of the paylo
    static constexpr size_t s_payload_size = 64;

    // Constructor
    MARE_MSC_IGNORE_BEGIN(4351);
    entry():
      _event_count(0),
      _event_id(0),
      _thread_id(),
      _buffer() { }
    MARE_MSC_IGNORE_END(4351);

    // Resets entry with a new count, event id, and thread id.
    // Notice that we don't clean the payload, because we overwrite
    // it when we add a new event.
    void reset(event_context::counter_type c, event_id e, std::thread::id id) {
      _event_count = c;
      _event_id = e;
      _thread_id = id;
    }

     // Returns event id.
    event_context::counter_type get_event_count() const {
      return _event_count;
    }

    // Returns event id.
    event_id  get_event_id() const { return _event_id; }

    // Returns thread id.
    std::thread::id get_thread_id() const { return _thread_id; };

    // Returns pointer to buffer location
    void* get_buffer() { return _buffer; }

  private:
    //Total counter
    event_context::counter_type _event_count;

    // ID of the event.
    event_id  _event_id;

    // ID of the thread that caused the event
    std::thread::id _thread_id;

    // Event payload
    char _buffer[s_payload_size];

  };//struct entry

private:

  // Checks whether buffer is empty.
  static bool is_buffer_empty();

  // Returns the first entry in the buffer. Remember that the buffer
  // is circular, so the first entry might not always be on pos 0
  static event_context::counter_type get_first_entry_pos();

  // Returns a string with the thread ID.
  static std::string get_thread_name(std::thread::id thread_id);

  // Event buffer
  typedef std::array<entry, s_size> log_array;
  static log_array s_entries;

  // Thread map
  typedef std::map<std::thread::id, std::string> thread_map;
  static thread_map  s_threads;
  static std::thread::id s_main_thread;

  MARE_DELETE_METHOD(imlogger());
  MARE_DELETE_METHOD(imlogger(imlogger const&));
  MARE_DELETE_METHOD(imlogger(imlogger&&));
  MARE_DELETE_METHOD(imlogger& operator=(imlogger const&));
  MARE_DELETE_METHOD(imlogger& operator=(imlogger&&));


}; // class imlogger

} // namespace mare::internal::log
} // namespace mare::internal
} // namespace mare
