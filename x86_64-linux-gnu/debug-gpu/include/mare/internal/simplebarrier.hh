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

#include <memory>

namespace mare {
namespace internal {

/// @brief Basic, non high-performance barrier.
///
/// This is a *very* basic barrier class to do some simple stuff in
/// mare. If we need anything more performant, we should write
/// something different.
class simple_barrier
{
public:

  typedef std::shared_ptr<simple_barrier> ptr;

  /// Creates a simple_barrier that waits for numthreads and returns
  /// a shared pointer to it.
  /// @return Returns a shared pointer to the barrier.
  static ptr create(size_t numthreads) {
    return ptr(new simple_barrier(numthreads));
  }

  ~simple_barrier() {
  }

  /// Called by the threads to announce they arrived to the barrier.
  /// All threads wait until thread N arrives
  void arrived() {
    std::atomic_fetch_sub(&_numthreads, static_cast<size_t>(1));
    std::unique_lock<std::mutex> lock(_mutex);

    if (std::atomic_load(&_numthreads) == 0 ) {
      _condvar.notify_all();
    } else {
      _condvar.wait(lock, [&](){
          return std::atomic_load(&_numthreads) == 0;
        } );
    }
  }

private:

  /// Constructor. It is private so that programmers use simple_barrier::create
  /// instead.
  /// @param numthreads Number of threds the barrier waits for.
  simple_barrier(size_t numthreads):
    _mutex(),
    _condvar(),
    _numthreads(numthreads) {
    if (numthreads<2)
      MARE_FATAL("Simple barrier requires at least two threds");
  }

  std::mutex _mutex;
  std::condition_variable _condvar;
  std::atomic<size_t> _numthreads;
};

}; // namespace internal
}; // namespace mare
