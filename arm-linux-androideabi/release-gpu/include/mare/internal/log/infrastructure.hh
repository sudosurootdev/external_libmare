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

#include <utility>

#include <mare/internal/debug.hh>
#include <mare/internal/templatemagic.hh>

#include "events.hh"
#include "loggerbase.hh"
#include "objectid.hh"

namespace mare {
namespace internal {
namespace log {


// GCC 4.6 moans about base class having a non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");

/**
   These templates check whether any logger is enabled.
   If that's the case, then value is true. Otherwise, value
   is false
*/
template<typename ...LS> struct any_logger_enabled;
template<>
struct any_logger_enabled<> : public std::integral_constant<bool, false> { };

template<typename L1, typename...LS>
struct any_logger_enabled<L1, LS...> :
    public std::integral_constant<bool, (L1::enabled::value)?
                                  true :
                                  any_logger_enabled<LS...>::value> {
};

MARE_GCC_IGNORE_END("-Weffc++");

/**
   These templates are used to call log(), init(), and shutdown()
   on the enabled loggers
 */
template<typename ...T> struct method_dispatcher;

template<> struct method_dispatcher<> {
  template<typename EVENT>
  static void log(EVENT&&, event_context&){ };
  static void init(){ };
  static void shutdown(){ };
  static void dump(){ };
  static void pause(){ };
  static void resume(){ };
};

template<typename L1, typename...LS>
struct method_dispatcher<L1, LS...>
{
private:

  template<typename EVENT>
  static void _log(EVENT&& e, event_context& context, std::true_type) {
    L1::log(std::forward<EVENT>(e), context);
  }

  template<typename EVENT>
  static void _log(EVENT&&, event_context&, std::false_type) {}

public:

  template<typename EVENT>
  static void log(EVENT&& e, event_context& context) {
    _log(std::forward<EVENT>(e), context, typename L1::enabled());
    method_dispatcher<LS...>::log(std::forward<EVENT>(e), context);
  }

  static void init() {
    static_assert(std::is_base_of<logger_base, L1>::value,
                  "\nAll mare loggers must inherit from logger_base."
                  "\nPlease read loggerbase.hh to learn how to "
                  "write a logger");

    if (L1::enabled::value)
      L1::init();
    method_dispatcher<LS...>::init();
  }

  static void shutdown() {
    if (L1::enabled::value)
      L1::shutdown();
    method_dispatcher<LS...>::shutdown();
  }

  static void dump() {
    if (L1::enabled::value)
      L1::dump();
    method_dispatcher<LS...>::dump();
  }

  static void pause() {
    if (L1::enabled::value)
      L1::paused();
    method_dispatcher<LS...>::pause();
  }

  static void resume() {
    if (L1::enabled::value)
      L1::resumed();
    method_dispatcher<LS...>::resume();
  }
};



/**
   This template represents the MARE logging infrastructure. Each of
   the template parameters is a different logger.
 */
template <typename L1, typename ...LN>
class infrastructure {

private:

  // std::integral_constant<bool, true> if at least one of the loggers
  // in the system is enabled. Otherwise, std::integral_constant<bool,
  // false>.
  typedef typename any_logger_enabled<L1, LN...>::integral_constant
    any_logger_enabled;

  // Loggers in the system. We could have use a type list of some
  // sort, but this strategy worked and I felt it was just ok.
  typedef method_dispatcher<L1, LN...> loggers;

  // The logging infrastructure starts in UNITIALIZED state. It transitions
  // to ACTIVE on init(). It transitions to PAUSED on pause() and returns
  // to ACTIVE on resume(). Finally, it transitions to FINISHED
  // on shutdown().
  enum class status : unsigned {
    UNINITIALIZED,
    ACTIVE,
    PAUSED,
    FINISHED
  };
  static status s_status;

  // Called by event(EVENT&& e) when no logger is enabled, thus making
  // sure that the compiler can optimize everything away if all loggers
  // are disabled.
  template<typename EVENT>
  static void _event(EVENT&&, std::false_type) {}


  // Called by event(EVENT&& e) when at least one logger is enabled.
  // Returns immediately if the logging infrastructure is not in
  // ACTIVE mode. This means that we have a dynamic check for
  // each event logging.
  template<typename EVENT>
  static void _event(EVENT&& e, std::true_type) {
    if (s_status != status::ACTIVE)
      return;

    event_context context;
    loggers::log(std::forward<EVENT>(e), context);
  }

public:

  // This template allows us to choose the type of object_id that
  // we'll use. If no logger is enabled, we'll use
  // null_log_object_id. Otherwise, we'll use seq_log_object_id.
  typedef typename std::conditional<any_logger_enabled::value,
                                    seq_object_id<task>,
                                    null_object_id<task>>::type task_id;

  typedef typename std::conditional<any_logger_enabled::value,
                                    seq_object_id<group>,
                                    null_object_id<group>>::type group_id;

  // Initializes logging infrastructure by calling init() on all
  // active loggers.  It causes a transition to ACTIVE state.  It
  // should only be called once. Subsequent calls to init() will be
  // ignored.
  static void init() {
    if(any_logger_enabled::value == false ||
       s_status != status::UNINITIALIZED)
      return;

    static_assert(duplicated_types<L1, LN...>::value == false,
                  "\nDuplicated logger types in logging infrastructure.");

    loggers::init();
    s_status = status::ACTIVE;
  }

  // Shuts the logging infrastructure down by calling shutdown() on
  // all active loggers.  It causes a transition to FINISHED state.  It
  // should only be called once. Subsequent calls to shutdown() will
  // be ignored.
  static void shutdown() {
    if(any_logger_enabled::value == false ||
       s_status == status::UNINITIALIZED ||
       s_status == status::FINISHED)
      return;

    loggers::shutdown();
    s_status = status::FINISHED;
  }

  // Pauses logging. Causes a transtition to PAUSED state.
  static void pause() {
    if (any_logger_enabled::value == false ||
        s_status == status::PAUSED)
      return;
    s_status = status::PAUSED;
    loggers::pause();
  }

  // Resumes logging.  Causes a transtition to LOGGING state.
  static void resume() {
    if(any_logger_enabled::value == false ||
       s_status == status::ACTIVE)
      return;

    s_status = status::ACTIVE;
    loggers::resume();
  }

  // Returns true if the logging has been initialized but not shut
  // down yet.
  static bool is_initialized() {
    return (s_status == status::ACTIVE) || (s_status == status::PAUSED);
  }

  // Returns true if the logging is in ACTIVE state.
  static bool is_active() {
    return (s_status == status::ACTIVE);
  }

  // Returns true if the logging is in PAUSED state.
  static bool is_paused() {
    return (s_status == status::PAUSED);
  }

  static void dump() {
    loggers::dump();
  }

  // Logs event e.
  template<typename EVENT>
  static void event(EVENT&& e) {
    _event(std::forward<EVENT>(e), any_logger_enabled());
  }

  // Disable all construction, copying and movement
  MARE_DELETE_METHOD(infrastructure());
  MARE_DELETE_METHOD(infrastructure(infrastructure const&));
  MARE_DELETE_METHOD(infrastructure(infrastructure&&));
  MARE_DELETE_METHOD(infrastructure& operator=(infrastructure const&));
  MARE_DELETE_METHOD(infrastructure& operator=(infrastructure&&));
}; // class infrastructure

// Static members initialization
template <typename L1, typename ...LN>
typename infrastructure<L1, LN...>::status
  infrastructure<L1, LN...>::s_status =
  infrastructure::status::UNINITIALIZED;

} // mare::internal::log
} // mare::internal
} // mare
