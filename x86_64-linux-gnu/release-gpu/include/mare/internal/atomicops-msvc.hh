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
#include <mare/internal/debug.hh>

#if _M_X64
#define MARE_SIZEOF_MWORD 8
#else
#define MARE_SIZEOF_MWORD 4
#endif

#define MARE_HAS_ATOMIC_DMWORD 1
#define MARE_SIZEOF_DMWORD (MARE_SIZEOF_MWORD * 2)

namespace mare {
namespace internal {

// This is LONGLONG even for 16 byte types since users will want to
// split this up into bitfields.  The use is responslible to make sure
// the overal type is aligned
typedef LONGLONG mare_dmword_t;

static inline bool
msvc_cmpxchg_4(uint32_t volatile* ptr, uint32_t* exp, uint32_t des)
{
  uint32_t old;

  old = InterlockedCompareExchange(ptr, des, *exp);

  if ((*exp) == old) {
    return true;
  } else {
    // __atomic_compare_exchange changes the expected value if
    // there is a failure, so emulate this
    *exp = old;
    return false;
  }
}

static inline bool
msvc_cmpxchg_8(uint64_t volatile* ptr, uint64_t* exp, uint64_t des)
{
  LONGLONG old;
  LONGLONG llexp = *exp;
  LONGLONG lldes = des;
  LONGLONG volatile* llptr = reinterpret_cast<LONGLONG volatile*>(ptr);

  old = InterlockedCompareExchange64(llptr, lldes, llexp);

  if (llexp == old) {
    return true;
  } else {
    // __atomic_compare_exchange changes the expected value if
    // there is a failure, so emulate this
    *exp = old;
    return false;
  }
}

#if MARE_HAS_ATOMIC_DMWORD && MARE_SIZEOF_MWORD == 8
static inline bool
msvc_cmpxchg_16(LONGLONG volatile* ptr, LONGLONG* exp,
                  const LONGLONG *des)
{
  // cnr must be 16 byte aligned
  __declspec(align(16)) LONGLONG cnr[2] = { exp[0], exp[1] };
  // I don't know who to be angry at here, window or intel but hi is
  // the second machine word
  LONGLONG des_hi = des[1];
  LONGLONG des_lo = des[0];
  uint8_t ret;

  MARE_API_ASSERT((reinterpret_cast<uintptr_t>(ptr) & 0xfull) == 0,
                  "ptr must be 16 byte aligned");

  // make sure we call the one with the '_' prefix
  ret = _InterlockedCompareExchange128(ptr, des_hi, des_lo, cnr);
  if (ret)
    return true;

  exp[0] = cnr[0];
  exp[1] = cnr[1];
  return false;
}
#endif

template<size_t N, typename T>
inline bool
atomicops<N,T>::cmpxchg_unchecked(T* dest, T* expected,
                                  T const* desired, bool weak,
                                  std::memory_order, std::memory_order)
{
  static_assert(N == 4 || N == 8 ||
                (N == 16 && MARE_HAS_ATOMIC_DMWORD && MARE_SIZEOF_MWORD == 8),
                "unsupported size for atomicops");

  switch (N) {
  case 4:
    {
      uint32_t volatile* ptr = reinterpret_cast<uint32_t volatile*>(dest);
      uint32_t *exp = reinterpret_cast<uint32_t *>(expected);
      uint32_t des = *reinterpret_cast<const uint32_t *>(desired);

      return msvc_cmpxchg_4(ptr, exp, des);
    }
    break;
  case 8:
    {
      uint64_t volatile* ptr = reinterpret_cast<uint64_t volatile*>(dest);
      uint64_t *exp = reinterpret_cast<uint64_t *>(expected);
      uint64_t des = *reinterpret_cast<const uint64_t *>(desired);

      return msvc_cmpxchg_8(ptr, exp, des);

    }
    break;
#if MARE_HAS_ATOMIC_DMWORD && MARE_SIZEOF_MWORD == 8
  case 16:
    {
      LONGLONG volatile* ptr = reinterpret_cast<LONGLONG volatile*>(dest);
      LONGLONG* exp = reinterpret_cast<LONGLONG*>(expected);
      const LONGLONG* des = reinterpret_cast<LONGLONG const*>(desired);

      return msvc_cmpxchg_16(ptr, exp, des);
    }
    break;
#endif
  }
  MARE_UNREACHABLE("Unexpected size encountered");
}

template<size_t N, typename T>
inline void
atomicops<N,T>::load_unchecked(T const* src, T* dest, std::memory_order)
{
  static_assert(N == 4 || N == 8 ||
                (N == 16 && MARE_HAS_ATOMIC_DMWORD && MARE_SIZEOF_MWORD == 8),
                "unsupported size for atomicops");

  switch (N) {
  case 4:
  case 8:
    MemoryBarrier();
    *dest = *src;
    break;
#if MARE_HAS_ATOMIC_DMWORD && MARE_SIZEOF_MWORD == 8
  case 16:
    {
      LONGLONG* ptr =
        const_cast<LONGLONG*>(
            reinterpret_cast<LONGLONG const*>(src));
      LONGLONG* dst = reinterpret_cast<LONGLONG*>(dest);

      // cnr needs to by 16 byte aligned
      __declspec(align(16)) LONGLONG cnr[2] = {0 ,0};

      MARE_API_ASSERT((reinterpret_cast<uintptr_t>(ptr) & 0xfull) == 0,
                      "ptr must be 16 byte aligned");

      // just go direct to the intrinsic which updates cnr _always_
      _InterlockedCompareExchange128(ptr, 0, 0, cnr);
      dst[0] = cnr[0];
      dst[1] = cnr[1];
    }
    break;
#endif
  default:
    MARE_UNREACHABLE("Unimplemented size");
    break;
  }
}

template<size_t N, typename T>
inline void
atomicops<N,T>::store_unchecked(T* dest, T const* desired, std::memory_order)
{
  static_assert(N == 4 || N == 8 ||
                (N == 16 && MARE_HAS_ATOMIC_DMWORD && MARE_SIZEOF_MWORD == 8),
                "unsupported size for atomicops");
  switch (N) {
  case 4:
  case 8:
    *dest = *desired;
    MemoryBarrier();
    break;
#if MARE_HAS_ATOMIC_DMWORD && MARE_SIZEOF_MWORD == 8
  case 16:
    {
      LONGLONG const* des = reinterpret_cast<LONGLONG const*>(desired);
      LONGLONG volatile* ptr = reinterpret_cast<LONGLONG volatile*>(dest);

      for (;;) {
        LONGLONG exp[2] = { ptr[0], ptr[1] };
        if (msvc_cmpxchg_16(ptr, exp, des))
          return;
      }
    }
    break;
#endif
  default:
    MARE_UNREACHABLE("Unimplemented size");
    break;
  }
}

static inline void
mare_atomic_thread_fence(std::memory_order m)
{
  std::atomic_thread_fence(m);
}

}; // namespace internal
}; // namespace mare
