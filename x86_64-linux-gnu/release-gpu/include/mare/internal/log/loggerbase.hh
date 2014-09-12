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

#include <mare/internal/log/events.hh>
#include <mare/internal/log/objectid.hh>

/**
   All loggers must inherit from logger_base.

   The class is empty now, but that might change
   in the future.
 */
namespace mare {
namespace internal {
namespace log {

  class logger_base {
  public:
    virtual ~logger_base(){}
  };

} //mare::log
} //mare::internal
} //mare



// Use the following code as a blueprint for your own logger.

// #pragma once
//
// #include "loggerbase.hh"
// #include "events.hh"
//
// namespace mare {
// namespace internal {
// namespace log {
//
// struct my_logger : public logger_base {
//
// #ifndef MARE_USE_MYLOGGER
//   typedef std::false_type enabled;
// #else
//   typedef std::true_type enabled;
// #endif
//
//   static void log(task_destroyed&& event, event_context& context) {
//   ...
//   }
//
//   template<typename UnknownEvent>
//   static void log(UnknownEvent&& e, event_context& context) {
//   // do nothing
//   }
//
//   static void init() { }
//
//   static void shutdown() {
//   }
//
// // Logging infrastructure has been paused
//  static void paused(){};
//
//  // Logging infrastructure has been resumed
//  static void resumed(){};
//
//   MARE_DELETE_METHOD(mylogger());
//   MARE_DELETE_METHOD(mylogger(mylogger const&));
//   MARE_DELETE_METHOD(mylogger(mylogger&&));
//   MARE_DELETE_METHOD(mylogger& operator=(mylogger const&));
//   MARE_DELETE_METHOD(mylogger& operator=(mylogger&&));


// };

// } // namespace mare::internal::log
// } // namespace mare::internal
// } // namespace mare
