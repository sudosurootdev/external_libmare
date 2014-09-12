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

#include <mutex>
#include <random>

#include <mare/internal/debug.hh>

namespace mare {
namespace internal {

class random_seed {
public:
  // constructor that chooses seed for you is in runtime.cc
  random_seed();

  // this interface is used by other test harnesses, like googletest,
  // where a seed value is provided
  random_seed(unsigned int seed) :
    _seed(seed),
    _rgen(),
    _mutex() {
    _rgen.seed(_seed);
  }

  unsigned int get_seed() const { return _seed; }
  unsigned int operator()() {
    // Do we need a lock? Maybe, but this patch should never be
    // performance critical
    std::unique_lock<std::mutex> lock(_mutex);
    unsigned int seed;

    // some other generators don't like to get 0 for a seed so we make
    // sure never give one.
    while ((seed = _rgen()) == 0)
      continue;
    return seed;
  }

private:
  unsigned int _seed;
  std::default_random_engine _rgen;
  std::mutex _mutex;
};

}; // namespace internal

}; // namespace mare
