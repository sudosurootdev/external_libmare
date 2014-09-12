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

#include <atomic>
#include <thread>

#include <mare/internal/compat.h>
#include <mare/internal/debug.hh>

namespace mare {
namespace internal {

class task;
class group;

namespace log {

typedef unsigned int event_id;


/**
   Some operations are expensive. For example, geting the tie
   of the day or the thread id. If we have multiple loggers,
   we want to avoid having each logger calling them for each
   event. This class takes care of that.

   The logging infrastructure creates an event_context object and
   passes it to all the loggers. If several loggers must know the
   thread id, only the first one that calls get_this_thread_id will
   actually call std::this_thread::get_id(). The rest will read the
   cached result.
 */
class event_context {

public:

  typedef size_t counter_type;
  typedef uint64_t time_type;

  /**
     Constructor. Does nothing
  */
  event_context() :
    _count(0),
    _thread(),
    _when(0)
  {}

  /**
     Gets thread id of the calling thread. It only calls
     this_thread::get_id() once.
  */
  std::thread::id get_this_thread_id() {
    if (_thread == s_null_thread_id)
      _thread = std::this_thread::get_id();
    return _thread;
  }

  /**
     Gets time at which the first call to get_time happened.
  */
  time_type get_time() {
    if (_when == 0)
      _when = mare_get_time_now();
    return _when;
  }

  counter_type get_count() {
    if (_count == 0)
      _count = s_global_counter.fetch_add(1, std::memory_order_relaxed);

    return _count;
  }

private:

  counter_type _count;
  std::thread::id _thread;
  time_type _when;

  static std::thread::id s_null_thread_id;
  static std::atomic<counter_type> s_global_counter;

  // Disable all copying and movement, since other objects may have
  // pointers to the internal fields of this object.
  MARE_DELETE_METHOD(event_context(event_context const&));
  MARE_DELETE_METHOD(event_context(event_context&&));
  MARE_DELETE_METHOD(event_context& operator=(event_context const&));
  MARE_DELETE_METHOD(event_context& operator=(event_context&&));
};


// GCC moans about base class having a non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");

/**
   The following templates are used as boilerplates for all the
   possible events in the system.

   All events MUST inherit from base_event.
*/
template<event_id EVENT_ID>
struct base_event {

  static constexpr event_id ID = EVENT_ID;
  static constexpr event_id get_id() { return EVENT_ID; }

};

/**
    Many events only need to store a pointer to the object that caused
    it (i.e: task, group). Therefore, we use this base class template
    to represent them.
*/
template<event_id EVENT_ID, typename OBJECT_TYPE>
struct single_object_event : public base_event<EVENT_ID> {

  single_object_event(OBJECT_TYPE* object) :
  _object(object) {}

  // Returns object that caused event.
  OBJECT_TYPE* get_object() const {return _object; }

private:
  OBJECT_TYPE* const _object;

};

/** Base class for events that involve just one group. */
template <event_id EVENT_ID>
struct single_group_event : public single_object_event<EVENT_ID, group> {

  single_group_event(group* g) :
    single_object_event<EVENT_ID, group>(g) { }

  // Returns group that caused the event.
  group* get_group() const {
    return single_object_event<EVENT_ID, group>::get_object();
  }

};

/** Base class for group_reffed, group_unreffed events. */
template <event_id EVENT_ID>
struct group_ref_count_event : public single_group_event<EVENT_ID> {

  group_ref_count_event(group* g, size_t count) :
    single_group_event<EVENT_ID>(g),
    _count(count) {}

  // Returns new refcount for the group
  size_t get_count() const { return _count; }

private:
  const size_t _count;
};

/** Base class for events that involve just one task. */
template <event_id EVENT_ID>
struct single_task_event : public single_object_event<EVENT_ID, task> {

  single_task_event(task* g) :
    single_object_event<EVENT_ID, task>(g) { }

  // Returns task that caused the event
  task* get_task() const {
    return single_object_event<EVENT_ID, task>::get_object();
  }

};

/** Base class for class_reffed, class_unreffed events.*/
template <event_id EVENT_ID>
struct task_ref_count_event : public single_task_event<EVENT_ID> {

  task_ref_count_event(task* t, size_t count) :
    single_task_event<EVENT_ID>(t),
    _count(count) {}

  // Returns the value of the new ref_count for the tasks.
  size_t get_count() const { return _count; }

private:
  const size_t _count;

};

/** Base class for events that involve two tasks. */
template <event_id EVENT_ID>
struct dual_task_event : public single_task_event<EVENT_ID> {

  dual_task_event(task* t1, task* t2) :
    single_task_event<EVENT_ID>(t1),
    _other(t2) { }

  // Returns second task involved in event
  task* get_other_task() const { return _other;}

private:
  task* const _other;
};

/**
    Events related to ref_counting need to store a pointer to the object
    and a counter.
*/
template<event_id EVENT_ID>
struct object_ref_count_event : public base_event<EVENT_ID> {

  object_ref_count_event(void* o, size_t count) :
    _o(o),
    _count(count)
  {}

  // Returns object whose refcount changed
  void* get_object() const {return _o; }

  // Returns new refcount
  size_t get_count() const {return _count; }

private:
  void* const _o;
  const size_t _count;
};

} //mare::internal::log
} //mare::internal
} //mare

MARE_GCC_IGNORE_END("-Weffc++");
