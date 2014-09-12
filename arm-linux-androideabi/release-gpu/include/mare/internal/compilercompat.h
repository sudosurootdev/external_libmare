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
// Define a set macros that are used for compilers compatibility

#ifdef __GNUC__
#define MARE_GCC_PRAGMA(x) _Pragma(#x)
#define MARE_GCC_PRAGMA_DIAG(x) MARE_GCC_PRAGMA(GCC diagnostic x)
#define MARE_GCC_IGNORE_BEGIN(x) MARE_GCC_PRAGMA_DIAG(push)             \
  MARE_GCC_PRAGMA_DIAG(ignored x)
#define MARE_GCC_IGNORE_END(x) MARE_GCC_PRAGMA_DIAG(pop)
#ifdef __clang__
#define MARE_CLANG_PRAGMA_DIAG(x) MARE_GCC_PRAGMA(clang diagnostic x)
#define MARE_CLANG_IGNORE_BEGIN(x) MARE_CLANG_PRAGMA_DIAG(push)         \
  MARE_CLANG_PRAGMA_DIAG(ignored x)
#define MARE_CLANG_IGNORE_END(x) MARE_CLANG_PRAGMA_DIAG(pop)
#else
#define MARE_CLANG_IGNORE_BEGIN(x)
#define MARE_CLANG_IGNORE_END(x)
#endif // __clang__
#else
#define MARE_GCC_IGNORE_BEGIN(x)
#define MARE_GCC_IGNORE_END(x)
#define MARE_CLANG_IGNORE_BEGIN(x)
#define MARE_CLANG_IGNORE_END(x)
#endif // __GNUC__

#ifdef _MSC_VER
#define MARE_MSC_PRAGMA(x) __pragma(x)
#define MARE_MSC_PRAGMA_WARNING(x) MARE_MSC_PRAGMA(warning(x))
#define MARE_MSC_IGNORE_BEGIN(x) MARE_MSC_PRAGMA_WARNING(push)          \
  MARE_MSC_PRAGMA_WARNING(disable : x)
#define MARE_MSC_IGNORE_END(x) MARE_MSC_PRAGMA_WARNING(pop)
#else
#define MARE_MSC_IGNORE_BEGIN(x)
#define MARE_MSC_IGNORE_END(x)
#endif
