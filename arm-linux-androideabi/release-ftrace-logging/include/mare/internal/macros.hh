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

// Macro to suppress warnings about unused variables
#define MARE_UNUSED(x) ((void)x)

// Macro to define the proper incantation of inline for C files
#ifndef MARE_INLINE
#ifdef _MSC_VER
#define MARE_INLINE __inline
#else
#define MARE_INLINE inline
#endif // _MSC_VER
#endif

// Macro to define the proper constexpr for different compilers
#ifndef MARE_CONSTEXPR
#ifdef _MSC_VER
#define MARE_CONSTEXPR
#else
#define MARE_CONSTEXPR constexpr
#endif // _MSC_VER
#endif // MARE_CONSTEXPR

// and the replacement with const
#ifndef MARE_CONSTEXPR_CONST
#ifdef _MSC_VER
#define MARE_CONSTEXPR_CONST const
#else
#define MARE_CONSTEXPR_CONST constexpr
#endif // _MSC_VER
#endif // MARE_CONSTEXPR_CONST

// Macros to implement the alignment stuff
#ifdef _MSC_VER

#define MARE_ALIGN(size) __declspec(align(size))
#define MARE_ALIGNED(type, name, size) \
  __declspec(align(size)) type name

#else

#define MARE_ALIGN(size) __attribute__ ((aligned(size)))
#define MARE_ALIGNED(type, name, size) \
  type name __attribute__ ((aligned(size)))

#endif

// Macro to provide an interface for GCC-specific attributes, but that
// can be skipped on different compilers such as VS2012 that do not
// support these.
#ifdef _MSC_VER
#define MARE_GCC_ATTRIBUTE(args)
#else
#define MARE_GCC_ATTRIBUTE(args) __attribute__(args)
#endif

#ifdef __GNUC__
#define MARE_DEPRECATED(decl) decl __attribute__((deprecated))
#elif defined(_MSC_VER)
#define MARE_DEPRECATED(decl) __declspec(deprecated) decl
#else
#warning No MARE_DEPRECATED implementation for this compiler
#define MARE_DEPRECATED(decl) decl
#endif

// VS2012 does not support the noexcept keyword, so use a macro to
// provide different implementations
#ifdef _MSC_VER
#define MARE_NOEXCEPT
#else
#define MARE_NOEXCEPT noexcept
#endif

// VS2012 does not support explicitly deleted methods, so use a macro
// to provide different implementations
#ifdef _MSC_VER
// We do not provide an implementation here. This is ok, and if
// someone tries to call it, they will get a linker error.  You cannot
// put an implementation here, because some constructors need to
// specify initializers.
#define MARE_DELETE_METHOD(...) __VA_ARGS__
#else
// Use C++11 features to do this more cleanly
#define MARE_DELETE_METHOD(...) __VA_ARGS__ = delete
#endif

// VS2012 does not support explicit default constructors, so use a
// macro to provide different implementations
#ifdef _MSC_VER
#define MARE_DEFAULT_METHOD(...) __VA_ARGS__ { }
#else
#define MARE_DEFAULT_METHOD(...) __VA_ARGS__ = default
#endif

// Macro to allow detection of VS2012 with Nov 2012 CTP update
#define MARE_VS2012_NOV_CTP 170051025
