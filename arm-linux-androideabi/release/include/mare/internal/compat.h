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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Macro to define the proper incantation of inline for C files
#ifndef MARE_INLINE
#ifdef _MSC_VER
#define MARE_INLINE __inline
#else
#define MARE_INLINE inline
#endif // _MSC_VER
#endif

#ifdef _MSC_VER
// Include Windows headers, disable min() and max() macros
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

// Disable constexpr since it is not supported in VS2012
#define constexpr const

// Provide the equivalent GCC intrinsic for VS2012
static MARE_INLINE void __sync_synchronize (void)
{
  MemoryBarrier();
}
#endif // _MSC_VER


#ifdef _MSC_VER
// Implement ssize_t for VS2012
typedef SSIZE_T ssize_t;
#else
#include <inttypes.h>
#endif // _MSC_VER


#if defined(__ANDROID__) || defined(__linux__)
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

#if defined(__ANDROID__)
#include <mare/internal/compilercompat.h>
// ignore warning of static int stat() shadowing struct stat in Android headers
MARE_GCC_IGNORE_BEGIN("-Wshadow")
#include <sys/stat.h>
MARE_GCC_IGNORE_END("-Wshadow")
#elif defined(__linux__)
#include <sched.h>
#else
// Currently we only provide support for pinning threads on Android and Linux.
#endif

#include <mare/internal/macros.hh>
#include <mare/internal/popcount.h>

uint64_t mare_get_time_now();

#ifdef _MSC_VER
__declspec(noreturn) void mare_exit(int);
#else
void mare_exit(int) MARE_GCC_ATTRIBUTE ((noreturn));
#endif // _MSC_VER

void
mare_android_logprintf(int level, const char *format, ...)
  MARE_GCC_ATTRIBUTE ((format (printf, 2, 3)));

#ifdef __cplusplus
// Check there are no min or max macros defined which interfere with
// our std:: usage
#ifdef min
#ifdef _MSC_VER
static_assert(false, "Cannot use std::min when there is an existing min "
              "#define, on VS2012 you should define NOMINMAX when "
              "including Windows.h");
#else
#error Cannot use std::min when there is an existing min #define
#endif // _MSC_VER
#endif // min
#ifdef max
#ifdef _MSC_VER
static_assert(false, "Cannot use std::max when there is an existing min "
              "#define, on VS2012 you should define NOMINMAX "
              "when including Windows.h");
#else
#error Cannot use std::max when there is an existing max #define
#endif // _MSC_VER
#endif // max
#endif // __cplusplus

/// static_assert(constexpr,...) does not work on VS2012, because it
/// does not support constexpr. So use a macro to evaluate at runtime.
#ifdef _MSC_VER
#define mare_constexpr_static_assert(cond, format, ...) \
  do { if (!cond) MARE_FATAL(format, ## __VA_ARGS__); } while (0)
#else
#define mare_constexpr_static_assert static_assert
#endif


#ifdef _MSC_VER
// VS2012 does not provide unistd.h and usleep() or sleep(), so
// provide a replacement
#ifdef __cplusplus
static inline void usleep(size_t ms) { Sleep(static_cast<DWORD>(ms) / 1000); }
static inline void sleep(size_t s) { Sleep(static_cast<DWORD>(s) * 1000); }
#else
static void usleep(size_t ms) { Sleep((DWORD)ms / 1000); }
static void sleep(size_t s) { Sleep((DWORD)s * 1000); }
#endif // __cplusplus
#else
#include <unistd.h>
#endif // _MSC_VER


#ifdef _MSC_VER
// Provide a replacement for pthread TLS support on VS2012
typedef DWORD pthread_key_t;

static MARE_INLINE int pthread_key_create (pthread_key_t *key,
                                      void (*destructor)(void*))
{
  *key = TlsAlloc();
  if (*key == TLS_OUT_OF_INDEXES)
    return 1;
  else
    return 0;
}

static MARE_INLINE int pthread_key_delete (pthread_key_t key)
{
  // TlsFree returns 0 on failure, but pthread_key_delete is 0 on success
  return !TlsFree(key);
}

static MARE_INLINE void* pthread_getspecific(pthread_key_t key)
{
  return TlsGetValue(key);
}

static MARE_INLINE
int pthread_setspecific(pthread_key_t key, const void *value)
{
  // TlsSetValue returns 0 on failure, but pthread_setspecific is 0 on success
  return !TlsSetValue(key, (LPVOID)value);
}
#endif // _MSC_VER

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __cplusplus
#include <string>
#ifdef _MSC_VER
static inline std::string
mare_strerror(int errnum_)
{
  char buf[1024] = "";
  strerror_s(buf, errnum_);
  return buf;
}
#elif __APPLE__ || __ANDROID__ || \
  (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! defined _GNU_SOURCE
#include <cstring>
static inline std::string
mare_strerror(int errnum_)
{
  char buf[1024] = "";
  ::strerror_r(errnum_, buf, sizeof buf);
  return buf;
}
#else
#include <cstring>
static inline std::string
mare_strerror(int errnum_)
{
  char buf[1024] = "";
  return ::strerror_r(errnum_, buf, sizeof buf);
}
#endif // _MSC_VER
#endif // __cplusplus
