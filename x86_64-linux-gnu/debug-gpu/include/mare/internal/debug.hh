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

// Define a series of macros that are used for debugging and error reporting.
// We define our own macros with different names than what Android uses, so
// that we can easily intercept the calls and do anything we might need.
//    MARE_DLOG - debugging log
//    MARE_ILOG - information log
//    MARE_WLOG - warning log
//    MARE_FATAL - error log, with exit(1) terminating the application
//    MARE_ALOG - always log, this will print even when NDEBUG is enabled, used in timing reports
//    MARE_LLOG - same as always log, but will not print thread prefix info
//   Do not include an ending \n char, since that is provided automatically

#include <mare/internal/compilercompat.h>

#ifdef __ANDROID__
// On Android, since they have non-standard includes, we ignore a bunch of
// compiler warnings for gcc versions > 4.8, e.g. -Wshadow and -Wold-style-cast
MARE_GCC_IGNORE_BEGIN("-Wshadow")
MARE_GCC_IGNORE_BEGIN("-Wold-style-cast")
#endif // __ANDROID__
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <atomic>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include <mare/internal/compat.h>

#ifdef __ANDROID__
MARE_GCC_IGNORE_END("-Wold-style-cast")
MARE_GCC_IGNORE_END("-Wshadow")
#endif // __ANDROID__

#include <mare/internal/memdebug.hh>
#include <mare/internal/macros.hh>
#include <mare/internal/tls.hh>

namespace mare {

namespace internal {

uintptr_t get_thread_id();

using ::mare_exit;
using ::mare_android_logprintf;

// use regular ints to make the printing code as portable as possible.
// use %8x for printing alignment, but if the system has a 64-bit int
// then this will eventually exceed that, which is not a problem.
extern std::atomic<unsigned int> mare_log_counter;

}; // namespace internal

}; // namespace mare

#ifdef _MSC_VER
// Function to take a format string and return a temporary string that
// can be passed into VS2012 *printf functions.  Note that since we
// return an std::string, we can call .c_str() from a macro and pass
// it off for printing and the memory will correctly be freed later.
#include <string>
inline const std::string mare_c99_vsformat (const char* _format)
{
  bool last_pct = false;
  std::string fmtcopy (_format);
  for (auto& i : fmtcopy) {
    if (i == '%') {
      // If previous char was % as well, then disable the
      // flag. Otherwise set the flag.
      last_pct = !last_pct;
    } else {
      if (last_pct) {
        if (i == 'z') {
          // We found a %z, which should be replaced with %I on VS2012
          i = 'I';
          last_pct = false;
        } else if (isdigit(i)) {
          // We found a digit after the %, so pass this through, and
          // stay in % mode
        } else {
          // We found something non-%, so we just pass this through,
          // but turn off % mode
          last_pct = false;
        }
      }
    }
  }
  return fmtcopy;
}
#endif // _MSC_VER

// Note that fetch_add(1,std::memory_order_relaxed) is safe to use since
// there are no other variables involved, and the compiler still uses
// ldrex/strex on ARM for example. But we can avoid the extra barriers.
#ifdef __ANDROID__
// Macros for Android platform
#define MARE_FMT_TID "x"
#define  MARE_DLOG(format, ...) mare::internal::mare_android_logprintf (ANDROID_LOG_DEBUG, "\033[32mD %8x t%" MARE_FMT_TID " %s:%d " format "\033[0m", mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__ , ## __VA_ARGS__)
#define  MARE_DLOGN(format, ...) mare::internal::mare_android_logprintf (ANDROID_LOG_DEBUG, "\033[32mD %8x t%" MARE_FMT_TID " %s:%d " format "\033[0m", mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__ , ## __VA_ARGS__)
#define  MARE_ILOG(format, ...) mare::internal::mare_android_logprintf (ANDROID_LOG_INFO,  "\033[33mI %8x t%" MARE_FMT_TID " %s:%d " format "\033[0m", mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__ , ## __VA_ARGS__)
#define  MARE_WLOG(format, ...) mare::internal::mare_android_logprintf (ANDROID_LOG_WARN,  "\033[35mW %8x t%" MARE_FMT_TID " %s:%d " format "\033[0m", mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__ , ## __VA_ARGS__)
#define  MARE_EXIT_FATAL(format, ...) mare::internal::mare_android_logprintf (ANDROID_LOG_ERROR, "\033[31mFATAL %8x t%" MARE_FMT_TID " %s:%d %s() " format, mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__, __FUNCTION__ , ## __VA_ARGS__), mare::internal::mare_android_logprintf (ANDROID_LOG_ERROR, "t%" MARE_FMT_TID " %s:%d **********", mare::internal::get_thread_id(), __FILE__, __LINE__), mare::internal::mare_android_logprintf (ANDROID_LOG_ERROR, "t%" MARE_FMT_TID " %s:%d - Terminating with exit(1)", mare::internal::get_thread_id(), __FILE__, __LINE__), mare::internal::mare_android_logprintf (ANDROID_LOG_ERROR, "t%" MARE_FMT_TID " %s:%d **********\033[0m", mare::internal::get_thread_id(), __FILE__, __LINE__), mare_exit(1)
#define  MARE_ALOG(format, ...) mare::internal::mare_android_logprintf (ANDROID_LOG_INFO,  "\033[36mA %8x t%" MARE_FMT_TID " %s:%d " format "\033[0m", mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__ , ## __VA_ARGS__)
#define  MARE_LLOG(format, ...) mare::internal::mare_android_logprintf (ANDROID_LOG_INFO, format "\n", ## __VA_ARGS__)
#else
// Macros for Linux/OSX platforms
#if defined(PRIxPTR)
#define MARE_FMT_TID PRIxPTR
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(__ARM_ARCH_7A__)
#define MARE_FMT_TID "x"
#else
#define MARE_FMT_TID "lx"
#endif
#ifdef _MSC_VER
// VS2012 does not support C99 format strings, so implement compatibility here
#define MARE_LOG_FPRINTF(stream, fmt, ...) fprintf(stream, mare_c99_vsformat(fmt).c_str(), ## __VA_ARGS__)
#else
#define MARE_LOG_FPRINTF(stream, fmt, ...) fprintf(stream, fmt, ## __VA_ARGS__)
#endif
#define  MARE_DLOG(format, ...) MARE_LOG_FPRINTF(stderr,"\033[32mD %8x t%" MARE_FMT_TID " %s:%d " format "\033[0m\n", mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__ , ## __VA_ARGS__)
#define  MARE_DLOGN(format, ...) MARE_LOG_FPRINTF(stderr,"\033[32mD %8x t%" MARE_FMT_TID " %s:%d " format "\033[0m", mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__ , ## __VA_ARGS__)
#define  MARE_ILOG(format, ...) MARE_LOG_FPRINTF(stderr,"\033[33mI %8x t%" MARE_FMT_TID " %s:%d " format "\033[0m\n", mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__ , ## __VA_ARGS__)
#define  MARE_WLOG(format, ...) MARE_LOG_FPRINTF(stderr,"\033[35mW %8x t%" MARE_FMT_TID " %s:%d " format "\033[0m\n", mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__ , ## __VA_ARGS__)
#define  MARE_EXIT_FATAL(format, ...) MARE_LOG_FPRINTF(stderr,"\033[31mFATAL %8x t%" MARE_FMT_TID " %s:%d %s() " format "\n", mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__, __FUNCTION__ , ## __VA_ARGS__), MARE_LOG_FPRINTF(stderr,"t%" MARE_FMT_TID " %s:%d - Terminating with exit(1)\033[0m\n", mare::internal::get_thread_id(), __FILE__, __LINE__), mare_exit(1)
#define  MARE_ALOG(format, ...) MARE_LOG_FPRINTF(stderr,"\033[36mA %8x t%" MARE_FMT_TID " %s:%d " format "\033[0m\n", mare::internal::mare_log_counter.fetch_add(1,std::memory_order_relaxed), mare::internal::get_thread_id(), __FILE__, __LINE__ , ## __VA_ARGS__)
#define  MARE_LLOG(format, ...) MARE_LOG_FPRINTF(stderr, format "\n", ## __VA_ARGS__)
#endif

// Various error handler macros. These do not throw exceptions and cannot be caught.
#define MARE_UNIMPLEMENTED(format, ...)  MARE_EXIT_FATAL("Unimplemented code in function '%s' at %s:%d " format, __FUNCTION__, __FILE__, __LINE__, ## __VA_ARGS__)
#define MARE_UNREACHABLE(format, ...)  MARE_EXIT_FATAL("Unreachable code triggered in function '%s' at %s:%d " format, __FUNCTION__, __FILE__, __LINE__, ## __VA_ARGS__)

// MARE_FATAL (immediate exit, does not throw an exception, always enabled)
// This macro is called when we have a fatal error to report, which should
// never be ignored. MARE_FATAL does not work like an assert; you need to
// write the if statement yourself, so that it is obvious that this code
// is always checked. All the other asserts can be optionally disabled.
// MARE_FATAL should be used for out-of-memory, file-not-found, etc., that
// is not a bug in the code.
#define MARE_FATAL(format, ...) MARE_EXIT_FATAL(format, ## __VA_ARGS__)


// MARE_INTERNAL_ASSERT
// - Causes immediate exit
// - Disabled in NDEBUG, otherwise enabled by default.
// - Override with MARE_ENABLE_CHECK_INTERNAL or MARE_DISABLE_CHECK_INTERNAL
// Used for checking internal data structures and ensures the integrity
// of the library. This assert generally indicates a bug in the library
// itself and should be reported to the developers.
#ifndef NDEBUG
#define MARE_CHECK_INTERNAL
#endif

#ifdef MARE_DISABLE_CHECK_INTERNAL
#undef MARE_CHECK_INTERNAL
#endif // MARE_DISABLE_CHECK_INTERNAL

#ifdef MARE_ENABLE_CHECK_INTERNAL
#ifndef MARE_CHECK_INTERNAL
#define MARE_CHECK_INTERNAL
#endif
#endif // MARE_ENABLE_CHECK_INTERNAL

#ifdef MARE_CHECK_INTERNAL
#define MARE_INTERNAL_ASSERT_COND(cond) #cond
#define MARE_INTERNAL_ASSERT(cond, format, ...) do { if(!(cond)) { \
      MARE_EXIT_FATAL("Internal assert failed [%s] - " format, MARE_INTERNAL_ASSERT_COND(cond), ## __VA_ARGS__); } \
  } while(false)
#else
#define MARE_INTERNAL_ASSERT(cond, format, ...) do { \
    if(false) { if(cond){} if(format){} } } while(false)
#endif // MARE_CHECK_INTERNAL

// Control how logging is performed based on the compilation flags
#ifdef NDEBUG
// Disable some logging when NDEBUG is enabled
#ifndef MARE_NOLOG
# define MARE_NOLOG
#endif
#endif // NDEBUG

#ifdef MARE_NOLOG
# undef  MARE_DLOG
# undef  MARE_DLOGN
# undef  MARE_ILOG
# undef  MARE_WLOG
# define  MARE_DLOG(format, ...) do {} while (false)
# define  MARE_DLOGN(format, ...) do {} while (false)
# define  MARE_ILOG(format, ...) do {} while (false)
# define  MARE_WLOG(format, ...) do {} while (false)
#endif // MARE_NOLOG


// It is not possible to compile MARE with different compiler flags than the application that calls it.
// Internal data structures change size depending on them, and so you will get strange behavior or a
// crash if you try to mix and match this. So we change the name of a variable in init() so MARE will not
// link if you are not consistent with compiler options.
#ifdef MARE_CHECK_API
#define MARE_SYMBOL_API _api
#else
#define MARE_SYMBOL_API _no_api
#endif

#ifdef MARE_CHECK_API_DETAILED
#define MARE_SYMBOL_APID _apid
#else
#define MARE_SYMBOL_APID _no_apid
#endif

#ifdef MARE_CHECK_INTERNAL
#define MARE_SYMBOL_INT _int
#else
#define MARE_SYMBOL_INT _no_int
#endif

#ifdef NDEBUG
#define MARE_SYMBOL_NDEBUG _release
#else
#define MARE_SYMBOL_NDEBUG _debug
#endif

// These macros are needed to correctly concatenate the final variable name together
#define MARE_MSL_INTERNAL(x, y) x ## y
#define MARE_MSL_GLUE(x,y) MARE_MSL_INTERNAL(x,y)

#define MARE_SYMBOL_PROTECTION(name) MARE_MSL_GLUE(__mare_with, MARE_MSL_GLUE(MARE_SYMBOL_API, MARE_MSL_GLUE(MARE_SYMBOL_APID, MARE_MSL_GLUE(MARE_SYMBOL_INT, MARE_MSL_GLUE(MARE_SYMBOL_NDEBUG, MARE_MSL_GLUE(_, name))))))


