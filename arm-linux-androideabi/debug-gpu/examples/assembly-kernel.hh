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

#if !defined(__clang__) && !defined (_MSC_VER)
__attribute__((optimize("unroll-loops")))
#endif
void kernel() {

  volatile int a[64] = {0};
  int v = 0;
  int s = 0;

  for(int i=0; i < 64; ++i) {
#if defined(__arm__)
    asm volatile("mov %0, #3\t\nmov %1, #4" : "=r" (v), "=r" (s));
#elif defined(__x86_64)
    asm volatile("mov $3, %0\t\nmov $4, %1" : "=r" (v), "=r" (s));
#else
    v = i;
    s = i+1;
#endif
    a[i] = v + s;
  }
  (void) a;
}
