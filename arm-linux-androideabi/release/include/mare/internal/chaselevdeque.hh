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

// for the mare_atomic_thread_fence
#include <mare/internal/atomicops.hh>

/// set to 1 to serialize the queues for debugging purposes
#define MARE_CLD_SERIALIZE 0

/// set to 1 to force sequential conisitency
#define MARE_CLD_FORCE_SEQ_CST 0

#if MARE_CLD_SERIALIZE
#define MARE_CLD_LOCK std::lock_guard<std::mutex> lock(_serializing_mutex)
#else
#define MARE_CLD_LOCK do { } while(0)
#endif

#if MARE_CLD_FORCE_SEQ_CST
#define MARE_CLD_MO(x) std::memory_order_seq_cst
#else
#define MARE_CLD_MO(x) x
#endif

template <typename T>
class chase_lev_array {
public:
  chase_lev_array(unsigned logsz = 12) :
    _logsz(logsz),
    _arr(new std::atomic<T>[1ULL << logsz]) { }

  ~chase_lev_array() { delete _arr; }

  chase_lev_array(T&) { }

  size_t size() {
    return 1ULL << _logsz;
  }

  T get(size_t i,
        std::memory_order mo = MARE_CLD_MO(std::memory_order_relaxed)) {
    return _arr[i & (size() - 1)].load(mo);
  }

  void put(size_t i, T x,
           std::memory_order mo = MARE_CLD_MO(std::memory_order_relaxed)) {
    _arr[i & (size() - 1)].store(x, mo);
  }

  chase_lev_array<T> *resize(size_t bot, size_t top) {
    auto a = new chase_lev_array<T>(_logsz + 1);

    MARE_INTERNAL_ASSERT(top <= bot, "oops");

    for (size_t i = top; i < bot; ++i)
      a->put(i, get(i));
    return a;
  }

private:
  unsigned _logsz;
  std::atomic<T>* _arr;

  // OMG!! g++ shut up already!!
  chase_lev_array<T> & operator=(const chase_lev_array<T>&);
  chase_lev_array<T>(const chase_lev_array<T>&);
};

template <typename T, bool LEPOP = true>
class chase_lev_deque {
public:

  static constexpr size_t ABORT = ~0UL;

  chase_lev_deque(unsigned logsz = 12):
    _top(1), // -jx 0?
    _bottom(1),
    _cl_array(new chase_lev_array<T>(logsz)),
    _serializing_mutex()  { }

  // Eventually this class ends up subclassed by one with lots of
  // virtual methods I think this is a memory leak, but it shuts G++
  // up.
  virtual ~chase_lev_deque() {
    auto a = _cl_array.load();
    delete a;
  }

  size_t take(T& x) {
    MARE_CLD_LOCK;

    //should be acquire?
    size_t b = _bottom.load(MARE_CLD_MO(std::memory_order_relaxed));
    ///
    /// If we take() before the first push() then b will wrap on the
    /// store below and if a steal() happens we it will succeed and
    /// thing that it has grabbed something.
    ///
    /// Not sure what will happens when we properly wrap to 0?
    ///
    if (b == 0)
      return 0;

    b = _bottom.load(MARE_CLD_MO(std::memory_order_relaxed));
    auto a = _cl_array.load(MARE_CLD_MO(std::memory_order_relaxed));

    --b;
    // this should be a release and we can get rid of the fence
    _bottom.store(b, MARE_CLD_MO(std::memory_order_relaxed));
    mare::internal::mare_atomic_thread_fence(
        MARE_CLD_MO(std::memory_order_seq_cst));

    size_t t = _top.load(MARE_CLD_MO(std::memory_order_relaxed));

    if (b < t) {
      // -JX Chase-Lev says t but Le-Pop says b+1?
      if (LEPOP)
        _bottom.store(b + 1, MARE_CLD_MO(std::memory_order_relaxed));
      else
        _bottom.store(t, MARE_CLD_MO(std::memory_order_relaxed));
      return 0;
    }

    // Non-empty queue.
    x = a->get(b);
    if (b > t) {
      // more than one element
      return b - t;
    }

    size_t sz = 1;
    // Single last element in queue.
    // could use acquire_release instead of seq_cst
    if (!_top.compare_exchange_strong(
            t, t + 1,
            MARE_CLD_MO(std::memory_order_seq_cst),
            MARE_CLD_MO(std::memory_order_relaxed))) {
      // Failed race.
      sz = 0;
    }
    // -JX Chase-Lev says t+1, but Le-Pop says b+1
    if (LEPOP)
      _bottom.store(b + 1, MARE_CLD_MO(std::memory_order_relaxed));
    else
      _bottom.store(t + 1, MARE_CLD_MO(std::memory_order_relaxed));

    return sz;
  }

  size_t push(T x) {
    MARE_CLD_LOCK;
    size_t b = _bottom.load(MARE_CLD_MO(std::memory_order_relaxed));
    size_t t = _top.load(MARE_CLD_MO(std::memory_order_acquire));
    auto a = _cl_array.load(MARE_CLD_MO(std::memory_order_relaxed));

    if (b >= t + a->size()) {
      // Full queue.
      a = a->resize(b, t);
      _cl_array.store(a, MARE_CLD_MO(std::memory_order_relaxed));
    }
    a->put(b, x);
    mare::internal::mare_atomic_thread_fence(
        MARE_CLD_MO(std::memory_order_release));
    _bottom.store(b + 1, MARE_CLD_MO(std::memory_order_relaxed));
    return b - t;
  }

  size_t steal(T& x) {
    MARE_CLD_LOCK;
    size_t t = _top.load(MARE_CLD_MO(std::memory_order_acquire));
    mare::internal::mare_atomic_thread_fence(
        MARE_CLD_MO(std::memory_order_seq_cst));
    size_t b = _bottom.load(MARE_CLD_MO(std::memory_order_acquire));

    if (t >= b)
      return 0;

    // Non-empty queue.
    auto a = _cl_array.load(MARE_CLD_MO(std::memory_order_relaxed));
    x = a->get(t);
    if (!_top.compare_exchange_strong(
            t, t + 1,
            MARE_CLD_MO(std::memory_order_seq_cst),
            MARE_CLD_MO(std::memory_order_relaxed))) {
      // Failed race.
      return ABORT;
    }
    return b - t;
  }

  size_t unsafe_size() {
    auto t = _top.load();
    auto b = _bottom.load();

    return b - t;
  }

private:
  // try and keep top and bottom far apart.
  std::atomic<size_t> _top;
  std::atomic<size_t> _bottom;
  std::atomic<chase_lev_array<T>*> _cl_array;
  std::mutex _serializing_mutex;
};
