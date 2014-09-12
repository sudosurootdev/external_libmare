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

#include <atomic>

#include <mare/internal/macros.hh>
#include <mare/internal/atomicops.hh>

// GCC 4.6 moans about base class having a non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");

namespace mare {

namespace internal {

/// A std::atomic<T> aligned for use with compare_exchange_*
template <size_t N, typename T>
struct alignedatomic_n {
private:
#ifdef _MSC_VER
  static_assert(sizeof(N) <= 16, "Alignment is hard coded for MSVC");
  // it is not possible to pass N to the MSVC compiler
  MARE_ALIGNED (T, _v, 16);
#else
  MARE_ALIGNED (T, _v, N);
#endif // _MSC_VER
public:
  alignedatomic_n() : _v() {};
  MARE_DEFAULT_METHOD(~alignedatomic_n());

  alignedatomic_n(T v) : _v(v) {}

  operator T() const { return load(); }

  T load(std::memory_order m = std::memory_order_seq_cst) const {
    T tmp;
    internal::atomicops<N,T>::load(&_v, &tmp, m);
    return tmp;
  }

  /// NON-Atomically loads the value of the alignedatomic and returns it.
  /// @return The value of the alignedatomic.
  T loadNA() const {
    return _v;
  }

  bool compare_exchange_strong(
      T& e, T n,
      std::memory_order const& m = std::memory_order_seq_cst) {
    return internal::atomicops<N,T>::cmpxchg(&_v, &e, &n, false, m, m);
  }

  bool compare_exchange_weak(
      T& e, T n,
      std::memory_order const& m = std::memory_order_seq_cst) {
    return internal::atomicops<N,T>::cmpxchg(&_v, &e, &n, true, m, m);
  }

  void store(T const& n,
             std::memory_order const& m = std::memory_order_seq_cst) {
    internal::atomicops<N,T>::store(&_v, &n, m);
  }

  /// NON-Atomically stores the specified value into the alignedatomic.
  /// @param other The value to be stored into the alignedatomic
  /// @return The value of other.
  void storeNA(T const& n) {
    _v = n;
  }
};

// Disable the following 16-byte implementation since it is not used
// anywhere right now
#if 0
/// GCC 4.7.1 does not properly align atomic structs, hence all
/// CMPXCHG16B ops fail on x86-64 (where also load/store are
/// implemented in terms of dword).
template <typename T>
struct alignedatomic_n<16,T> : public std::atomic<T> {
  MARE_DEFAULT_METHOD(alignedatomic_n());
  MARE_DEFAULT_METHOD(~alignedatomic_n());

  /// NON-Atomically loads the value of the alignedatomic and returns it.
  /// @return The value of the alignedatomic.
  T loadNA() const {
    return this->load(std::memory_order_relaxed);
  }

  /// NON-Atomically stores the specified value into the alignedatomic.
  /// @param other The value to be stored into the alignedatomic
  /// @return The value of other.
  void storeNA(T const& other) {
    this->store(other, std::memory_order_relaxed);
  }
} __attribute__ ((aligned(16)));
#endif // 0

template <typename T>
struct alignedatomic : public alignedatomic_n<sizeof(T),T> {
  MARE_DEFAULT_METHOD(alignedatomic());
  MARE_DEFAULT_METHOD(~alignedatomic());
};


}; //namespace internal

}; //namespace mare

MARE_GCC_IGNORE_END("-Weffc++");
