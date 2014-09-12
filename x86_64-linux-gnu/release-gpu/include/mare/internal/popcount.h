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

#include <stdlib.h>
#include <stdint.h>

// Macro to define the proper incantation of inline for C files
#ifndef MARE_INLINE
#ifdef _MSC_VER
#define MARE_INLINE __inline
#else
#define MARE_INLINE inline
#endif // _MSC_VER
#endif

// http://openvswitch.org/pipermail/dev/2012-July/019297.html
// Alternative: HACKMEM 169
static MARE_INLINE int
mare_popcount(uint32_t x)
{
#ifdef HAVE___BUILTIN_POPCOUNT
  return __builtin_popcount(x);
#else
#define MARE_INIT1(X)                                                        \
  ((((X) & (1 << 0)) != 0) +                                            \
   (((X) & (1 << 1)) != 0) +                                            \
   (((X) & (1 << 2)) != 0) +                                            \
   (((X) & (1 << 3)) != 0) +                                            \
   (((X) & (1 << 4)) != 0) +                                            \
   (((X) & (1 << 5)) != 0) +                                            \
   (((X) & (1 << 6)) != 0) +                                            \
   (((X) & (1 << 7)) != 0))
#define MARE_INIT2(X)   MARE_INIT1(X),  MARE_INIT1((X) +  1)
#define MARE_INIT4(X)   MARE_INIT2(X),  MARE_INIT2((X) +  2)
#define MARE_INIT8(X)   MARE_INIT4(X),  MARE_INIT4((X) +  4)
#define MARE_INIT16(X)  MARE_INIT8(X),  MARE_INIT8((X) +  8)
#define MARE_INIT32(X) MARE_INIT16(X), MARE_INIT16((X) + 16)
#define MARE_INIT64(X) MARE_INIT32(X), MARE_INIT32((X) + 32)

  static const uint8_t popcount8[256] = {
  MARE_INIT64(0), MARE_INIT64(64), MARE_INIT64(128), MARE_INIT64(192)
    };

  return (popcount8[x & 0xff] +
          popcount8[(x >> 8) & 0xff] +
          popcount8[(x >> 16) & 0xff] +
          popcount8[x >> 24]);
#endif
          }

 static MARE_INLINE int
 mare_popcountl(unsigned long int x)
 {
#ifdef HAVE___BUILTIN_POPCOUNTL
   return __builtin_popcountl(x);
#else
   int cnt = mare_popcount((uint32_t)(x));
   if (sizeof x > 4)
     cnt += mare_popcount((uint32_t)(x >> 31 >> 1));
   return cnt;
#endif
 }

static MARE_INLINE int
mare_popcountw(size_t x)
{
  int cnt = mare_popcount((uint32_t)(x));
  if (sizeof x > 4)
    cnt += mare_popcount((uint32_t)(x >> 31 >> 1));
  return cnt;
}

#ifdef __cplusplus
}
#endif
