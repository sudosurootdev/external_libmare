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

#define MARE_SIZEOF_MWORD __SIZEOF_SIZE_T__
#define MARE_SIZEOF_DMWORD (MARE_SIZEOF_MWORD * 2)

#if __GNUC__ == 4 && __GNUC_MINOR__ == 7 && __GNUC_PATCHLEVEL__ == 0
  // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=48336
  // Error in generating ldrd
  // I believe it only effects this _exact_ version.
#define MARE_GCC_ARM_LDRD_BUG
#endif

#ifdef __thumb__
#define MARE_ARM_IF_THEN(c) "it " # c
#else
#define MARE_ARM_IF_THEN(c)
#endif

namespace mare {

namespace internal {

#ifdef __LP64__
// we don't have code for this yet
#define MARE_HAS_ATOMIC_DMWORD 0
typedef unsigned __int128 mare_dmword_t;
#else
#define MARE_HAS_ATOMIC_DMWORD 1
typedef uint64_t mare_dmword_t;
#endif

static inline void armv7a_inner_realm_barrier()
{
  // gcc 4.8 likes to use "dmb sy"
  // Do we need to throw out all of memory every time?
  asm volatile ("dmb ish":::"memory");
}

static inline bool
armv7a_cmpxchg_weak_4(uint32_t* ptr, uint32_t exp, uint32_t des,
                      uint32_t* old)
{
  uint32_t tmp;
  size_t failed_write;

  asm volatile ("@ armv7a_cmpxchg_weak_4 \n\t"
                "mov     %[fw], #1 \n\t"
                "ldrex   %[tmp], [%[ptr]] \n\t"
                "teq     %[tmp], %[exp] \n\t"
                MARE_ARM_IF_THEN(eq) " \n\t"
                "strexeq %[fw], %[des], [%[ptr]] \n"
                : [tmp] "=&r" (tmp),
                  [fw] "=&r" (failed_write),
                  "=Qo" (*ptr)
                : [ptr] "r" (ptr),
                  [exp] "r" (exp),
                  [des] "r" (des)
                : "cc", "memory");

  if (failed_write) {
    *old = tmp;
    return false;
  }
  return true;
}

static inline bool
armv7a_cmpxchg_strong_4(uint32_t* ptr, uint32_t exp, uint32_t des,
                        uint32_t* old)
{
  uint32_t tmp = 0;
  size_t failed_write;
  asm volatile ("@ armv7a_cmpxchg_strong_4 \n"
                "1: # loop \n"
                "        ldrex  %[tmp], [%[ptr]] \n"
                "        subs   %[fw], %[tmp], %[exp] \n"
                "        bne    2f \n"
                "        strex  %[fw], %[des], [%[ptr]] \n"
                "        teq    %[fw], #0 \n"
                "        bne    1b \n"
                "2: # out \n"
                : [tmp] "=&r" (tmp),
                  [fw] "=&r" (failed_write),
                  "=Qo" (*ptr)
                : [ptr] "r" (ptr),
                  [exp] "r" (exp),
                  [des] "r" (des)
                : "cc", "memory");

  if (failed_write) {
    *old = tmp;
    return false;
  }
  return true;
}

static inline bool
armv7a_cmpxchg_4(uint32_t* ptr, uint32_t* expected,
                uint32_t const* desired,
                bool weak,
                std::memory_order ms,
                std::memory_order)
{
  uint32_t exp = *expected;
  uint32_t des = *desired;
  uint32_t old = 0;
  bool done;

  if (weak) {
    done = armv7a_cmpxchg_weak_4(ptr, exp, des, &old);
  } else {
    done = armv7a_cmpxchg_strong_4(ptr, exp, des, &old);
  }

  if (ms == std::memory_order_seq_cst ||
      ms == std::memory_order_acquire ||
      ms == std::memory_order_acq_rel)
    armv7a_inner_realm_barrier();

  if (!done)
    *expected = old;
  return done;
}

static inline bool
armv7a_cmpxchg_weak_8(uint64_t* ptr, uint64_t exp, uint64_t des,
                      uint64_t* old)
{
  uint64_t tmp;
  size_t failed_write;

#ifdef MARE_GCC_ARM_LDRD_BUG
  tmp = __sync_val_compare_and_swap(ptr, exp, des);
  failed_write = (tmp != exp);
#else

  MARE_CLANG_IGNORE_BEGIN("-Wasm-operand-widths");
  asm volatile ("@ armv7a_cmpxchg_weak_8 \n\t"
                "ldrexd   %[tmp], %H[tmp], [%[ptr]] \n\t"
                "teq      %H[tmp], %H[exp] \n\t"
                MARE_ARM_IF_THEN(eq) " \n\t"
                "teqeq    %[tmp], %[exp] \n\t"
                MARE_ARM_IF_THEN(eq) " \n\t"
                "strexdeq %[fw], %[des], %H[des], [%[ptr]] \n\t"
                : [tmp] "=&r" (tmp),
                  [fw] "=&r" (failed_write),
                  "=Qo" (*ptr)
                : [ptr] "r" (ptr),
                  [exp] "r" (exp),
                  [des] "r" (des)
                : "cc", "memory");
  MARE_CLANG_IGNORE_END("-Wasm-operand-widths");
#endif

  if (failed_write) {
    *old = tmp;
    return false;
  }
  return true;
}

static inline bool
armv7a_cmpxchg_strong_8(uint64_t* ptr, uint64_t exp, uint64_t des,
                        uint64_t* old)
{
  uint64_t tmp;
  size_t failed_write;

#ifdef MARE_GCC_ARM_LDRD_BUG
  tmp = __sync_val_compare_and_swap(ptr, exp, des);
  failed_write = (tmp != exp);

#else

// clang erroneously complains about exp and des being the wrong width
  MARE_CLANG_IGNORE_BEGIN("-Wasm-operand-widths");

  asm volatile ("@ armv7a_cmpxchg_strong_8 \n"
                // we want fw to be 1 if we branch out the first time
                "        mov   %[fw], #1 \n"
                "1: #loop \n"
                "        ldrexd %[tmp], %H[tmp], [%[ptr]] \n"
                "        teq    %H[tmp], %H[exp] \n"
                "        " MARE_ARM_IF_THEN(eq) " \n"
                "        teqeq  %[tmp], %[exp] \n"
                "        bne    2f \n"
                "        strexd %[fw], %[des], %H[des], [%[ptr]] \n"
                "        teq    %[fw], #0 \n"
                "        bne    1b \n"
                "2: # exit \n"
                : [tmp] "=&r" (tmp),
                  [fw] "=&r" (failed_write),
                  "=Qo" (*ptr)
                : [ptr] "r" (ptr),
                  [exp] "r" (exp),
                  [des] "r" (des)
                : "cc", "memory");

  MARE_CLANG_IGNORE_END("-Wasm-operand-widths");

#endif

  if (failed_write) {
    *old = tmp;
    return false;
  }
  return true;
}

static inline bool
armv7a_cmpxchg_8(uint64_t* ptr, uint64_t* expected,
                 uint64_t const* desired,
                 bool weak,
                 std::memory_order ms,
                 std::memory_order)
{
  uint64_t exp = *expected;
  uint64_t des = *desired;
  uint64_t old = exp;
  bool done;

  if (weak) {
    done = armv7a_cmpxchg_weak_8(ptr, exp, des, &old);
  } else {
    done = armv7a_cmpxchg_strong_8(ptr, exp, des, &old);
  }

  if (ms == std::memory_order_seq_cst ||
      ms == std::memory_order_acquire ||
      ms == std::memory_order_acq_rel)
    armv7a_inner_realm_barrier();

  if (!done)
    *expected = old;
  return done;
}

template<size_t N, typename T>
inline bool
atomicops<N,T>::cmpxchg_unchecked(T* dest, T* expected,
                                  T const* desired, bool weak,
                                  std::memory_order msuccess,
                                  std::memory_order mfailure)
{

  // everyone gets this
  if (msuccess == std::memory_order_seq_cst ||
      msuccess == std::memory_order_release)
    armv7a_inner_realm_barrier();

  switch (N) {
  case 4:
    {
      uint32_t* ptr = reinterpret_cast<uint32_t*>(dest);
      uint32_t* exp = reinterpret_cast<uint32_t*>(expected);
      uint32_t const* des = reinterpret_cast<uint32_t const*>(desired);

      return armv7a_cmpxchg_4(ptr, exp, des, weak, msuccess, mfailure);
    }
    break;
  case 8:
    {
      uint64_t* ptr = reinterpret_cast<uint64_t*>(dest);
      uint64_t* exp = reinterpret_cast<uint64_t*>(expected);
      uint64_t const* des = reinterpret_cast<uint64_t const *>(desired);

      return armv7a_cmpxchg_8(ptr, exp, des, weak, msuccess, mfailure);
    }
    break;
  default:
    MARE_UNREACHABLE("Unimplemented size");
  }
}

static inline void
armv7a_load_4(uint32_t const*ptr, uint32_t *ret, std::memory_order m)
{
  uint32_t tmp;

  if (m == std::memory_order_seq_cst ||
      m == std::memory_order_acquire ||
      m == std::memory_order_consume)
    armv7a_inner_realm_barrier();

  asm volatile ("ldr %[tmp], [%[src]]"
                : [tmp] "=&r"(tmp)
                : [src] "r" (ptr));

  if (m == std::memory_order_seq_cst)
    armv7a_inner_realm_barrier();

  *ret = tmp;
}

static inline void
armv7a_load_8(uint64_t const* ptr, uint64_t* ret, std::memory_order m)
{
  uint64_t tmp;

#ifdef MARE_GCC_ARM_LDRD_BUG
  uint64_t* p = const_cast<uint64_t*>(ptr);
  tmp = __sync_val_compare_and_swap(p, 0ull, 0ull);
  MARE_UNUSED(m);

  // This barrier is necessary for the bug
  armv7a_inner_realm_barrier();

#else

  if (m == std::memory_order_seq_cst ||
      m == std::memory_order_acquire ||
      m == std::memory_order_consume)
    armv7a_inner_realm_barrier();

  asm volatile ("ldrd %[tmp], %H[tmp], [%[src]]"
                : [tmp] "=&r"(tmp)
                : [src] "r" (ptr));

  if (m == std::memory_order_seq_cst)
    armv7a_inner_realm_barrier();
#endif

  *ret = tmp;
}

template<size_t N, typename T>
inline void
atomicops<N,T>::load_unchecked(T const* src, T* dest, std::memory_order m)
{
  static_assert(N == 4 || N == 8,
                "atomicops only supports 4 or 8 byte");

  switch (N) {
  case 4:
    {
      uint32_t const*ptr = reinterpret_cast<uint32_t const*>(src);
      uint32_t *ret = reinterpret_cast<uint32_t*>(dest);

      armv7a_load_4(ptr, ret, m);
    }
    break;
  case 8:
    {
      uint64_t const*ptr = reinterpret_cast<uint64_t const*>(src);
      uint64_t *ret = reinterpret_cast<uint64_t*>(dest);
      armv7a_load_8(ptr, ret, m);
    }
    break;
  default:
    MARE_UNREACHABLE("Unimplemented size");
  }
}

static inline void
armv7a_store_4(uint32_t* ptr, uint32_t const* val, std::memory_order m)
{
  uint32_t v = *val;

  if (m == std::memory_order_seq_cst || m == std::memory_order_release)
    armv7a_inner_realm_barrier();

  *ptr = v;

  if (m == std::memory_order_seq_cst)
    armv7a_inner_realm_barrier();

}

static inline void
armv7a_store_8(uint64_t* ptr, uint64_t const* val, std::memory_order m)
{
  uint64_t tmp;
  uint64_t v = *val;

#ifdef MARE_GCC_ARM_LDRD_BUG
  MARE_UNUSED(m);
  for (;;) {
    tmp = *ptr;
    if (__sync_bool_compare_and_swap(ptr, tmp, v))
      break;
    // This causes the compiler to not keep memory values cached in
    // registers across the assembler instruction. It is used to make
    // sure that *ptr is reloaded into tmp
    asm volatile("" ::: "memory");
  }
#else
  if (m == std::memory_order_seq_cst || m == std::memory_order_release)
    armv7a_inner_realm_barrier();

  // Quote the ARM v7A ISA:
  //   Each 32-bit word access is guaranteed to be single-copy
  //   atomic. The architecture does not require subsequences of two
  //   or more word accesses from the sequence to be single-copy
  //   atomic.
  // Therefore we must use an exclusive load/store pair

  MARE_CLANG_IGNORE_BEGIN("-Wasm-operand-widths");

  asm volatile ("@ armv7a_store_8 \n"
                "1: # loop \n"
                "        ldrexd %[tmp], %H[tmp], [%[dst]] \n"
                "        strexd %[tmp], %[v], %H[v], [%[dst]] \n"
                "        teq    %[tmp], #0 \n"
                "        bne 1b \n\t"
                : [tmp] "=&r"(tmp),
                  "=Qo" (*ptr)
                : [dst] "r" (ptr),
                  [v] "r" (v)
                : "cc");
  MARE_CLANG_IGNORE_END("-Wasm-operand-widths");

  if (m == std::memory_order_seq_cst) {
    armv7a_inner_realm_barrier();
  }
#endif
}

template<size_t N, typename T>
inline void
atomicops<N,T>::store_unchecked(T* dest, T const* desired, std::memory_order m)
{
  static_assert(N == 4 || N == 8,
                "atomicops only supports 4 or 8 byte");

  switch (N) {
  case 4:
    {
      uint32_t* ptr = reinterpret_cast<uint32_t*>(dest);
      uint32_t const* val = reinterpret_cast<uint32_t const*>(desired);

      armv7a_store_4(ptr, val, m);
    }
    break;
  case 8:
    {
      uint64_t* ptr = reinterpret_cast<uint64_t*>(dest);
      uint64_t const* val = reinterpret_cast<uint64_t const*>(desired);

      armv7a_store_8(ptr, val, m);
    }
    break;
  default:
    MARE_UNREACHABLE("Unimplemented size");
  }

}

static inline void
mare_atomic_thread_fence(std::memory_order m)
{
  if (m != std::memory_order_relaxed)
    armv7a_inner_realm_barrier();
}

}; // namespace internal
}; // namespace mare
