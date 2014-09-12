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

namespace mare {

namespace internal {

typedef unsigned long int cpu_mask;

// Assume maximum of 128 cpus/cores.
static const size_t MARE_SET_SIZE = 128;
static const size_t MARE_NUM_CPU_BITS = (8 * sizeof(cpu_mask));
static const size_t MARE_NUM_BITSETS =
                                      (MARE_SET_SIZE % MARE_NUM_CPU_BITS == 0)?
                                      (MARE_SET_SIZE / MARE_NUM_CPU_BITS)
                                      :(MARE_SET_SIZE / MARE_NUM_CPU_BITS) + 1;

struct mare_cpu_set_t {
  cpu_mask bits[MARE_SET_SIZE/MARE_NUM_CPU_BITS];
};

static const size_t MARE_CPU_SET_SIZE = sizeof(mare_cpu_set_t);

void cpu_zero(mare_cpu_set_t* cpu_set_p);
void cpu_set(size_t cpu, mare_cpu_set_t* cpu_set_p);
size_t derive_cpu(mare_cpu_set_t* cpuset);
ssize_t set_thread_affinity(size_t cpu);
ssize_t get_thread_affinity(void);

}; // internal

}; // mare
