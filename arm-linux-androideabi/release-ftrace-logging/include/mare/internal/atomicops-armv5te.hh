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
#include <cstdint>
#include <climits>

extern "C" {
// some gcc's do not supply this function for armv5te
static inline uint64_t
__sync_val_compare_and_swap_8(uint64_t *ptr, uint64_t oldval, uint64_t newval)
{
  uint64_t tmp;

  asm volatile ("@ mare___sync_val_compare_and_swap_8 \n"
                // we want fw to be 1 if we branch out the first time
                "        ldrd   %[tmp], %H[tmp], [%[ptr]] \n"
                "        cmp    %H[tmp], %H[exp] \n"
                "        it     eq \n"
                "        cmpeq  %[tmp], %[exp] \n"
                "        bne    2f \n"
                "        strd   %[des], %H[des], [%[ptr]] \n"
                "2: # exit \n"
                : [tmp] "=&r" (tmp),
                  "=Qo" (*ptr)
                : [ptr] "r" (ptr),
                  [exp] "r" (oldval),
                  [des] "r" (newval)
                : "cc", "memory");
  return tmp;
}

static inline bool
__sync_bool_compare_and_swap_8(uint64_t *ptr, uint64_t oldval, uint64_t newval)
{
  uint64_t tmp;
  size_t failed_write;

  asm volatile ("@ mare___sync_val_compare_and_swap_8 \n"
                // we want fw to be 1 if we branch out the first time
                "        mov   %[fw], #1 \n"
                "        ldrd   %[tmp], %H[tmp], [%[ptr]] \n"
                "        cmp    %H[tmp], %H[exp] \n"
                "        it     eq \n"
                "        cmpeq  %[tmp], %[exp] \n"
                "        bne    2f \n"
                "        strd   %[des], %H[des], [%[ptr]] \n"
                "        mov   %[fw], #0 \n"
                "2: # exit \n"
                : [tmp] "=&r" (tmp),
                  [fw] "=&r" (failed_write),
                  "=Qo" (*ptr)
                : [ptr] "r" (ptr),
                  [exp] "r" (oldval),
                  [des] "r" (newval)
                : "cc", "memory");
  return !failed_write;
}

} // extern "C"
