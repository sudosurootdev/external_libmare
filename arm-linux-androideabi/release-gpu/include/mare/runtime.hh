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
/** @file runtime.hh */
#pragma once

#include <mare/common.hh>
#include <mare/internal/random.hh>
#include <mare/internal/runtime.hh>
#include <mare/internal/debug.hh>


namespace mare {

// The runtime namespace includes methods related to the MARE runtime and
// the MARE thread pool.
namespace runtime {

// See comments in runtime.cc for information about init_count
extern void init_implementation();
// we keep this as a protected symbols, which really means we
// sympolically mangle it in a way that will cause build breaks in the
// linker if the headers and the library objects are not in sync.
extern std::atomic<int> MARE_SYMBOL_PROTECTION(init_count);

/** @addtogroup init_shutdown
@{ */
/**
    Starts up MARE's runtime.

    Initializes MARE internal data structures, tasks schedulers, and
    thread pools. There should only be one call to init in the entire
    program, and it should be called before launching tasks or waiting
    for tasks or groups.
*/
inline void init()
{
  mare::runtime::MARE_SYMBOL_PROTECTION(init_count).fetch_add(1);
  init_implementation();
}

/**
    Shuts down MARE's runtime.

    Shuts down the runtime. It returns only when all running tasks
    have finished. Once the runtime has been shut down, it cannot be
    restarted.
*/
void shutdown();
/** @} */ /* end_addtogroup init_shutdown */

/** @addtogroup interop
@{ */
/**
    Callback function pointer.
*/
typedef void(*callback_t)();

/**
    Sets a callback function that MARE will invoke whenever it creates
    an internal runtime thread.

    Programmers should invoke this method before init() because init()
    may create threads. Programmers may reset the thread construction
    callback by using a nullptr as parameter to this method.

    @param fptr New callback function pointer

    @return Previous callback function pointer or nullptr if none.
*/
callback_t set_thread_created_callback(callback_t fptr);

/**
    Sets a callback function that MARE will invoke whenever it destroys
    an internal runtime thread.

    Programmers should invoke this method before init() because init()
    may destroy threads. Programmers may reset the thread destruction
    callback by using a nullptr as parameter to this method.
    @param fptr New callback function pointer.

    @return Previous callback function pointer or nullptr if none.
*/
callback_t set_thread_destroyed_callback(callback_t fptr);

/**
    Returns a pointer to the function that MARE will invoke whenever
    it creates a new thread.
    @return Current callback function pointer or nullptr if none.
*/
callback_t thread_created_callback();

/**
    Returns a pointer to the function that MARE will invoke whenever
    it destroys a new thread.
    @return Current callback function pointer or nullptr if none.
*/
callback_t thread_destroyed_callback();
/** @} */ /* end_addtogroup interop */

/** @cond HIDDEN */
extern mare::internal::random_seed* g_random_seed;

/** \brief An easy way to get to the global ::g_random_seed object */
#define MARE_RANDOM_SEED (*::mare::runtime::g_random_seed)

/**
 \brief Initializes the ::mare::runtime::random_seed object with a
 specific seed.  This is used by other test or runtime harnesses in
 order to make deterministic runs.

 @param seed The seed that should be used.
*/
static inline void
random_seed(unsigned int seed) {
  if (g_random_seed != nullptr)
    MARE_FATAL("g_random_seed already defined");

  // on some implementations, the default random number generator
  // cannot handle a seed of 0, so we make it the default for Mersenne
  // Twister
  if (seed == 0)
    seed = std::mt19937::default_seed;

  g_random_seed = new ::mare::internal::random_seed(seed);
}

/**
 \brief Initializes the ::mare::runtime::random_seed object and
 allows the ::runtime to select the seed.  You can easily reproduce
 this seed by using \c MARE_RANDOM_SEED environment variable.  You
 may also force MARE to print the number it will use by setting \c
 MARE_RANDOM_SEED to nothing, a non-number, or \c 0.

 \note This interface is called from the runtime, there is no need
 to call it externally, please see \see random_seed(int seed).
*/
static inline void
random_seed() {
  if (g_random_seed != nullptr)
    MARE_FATAL("g_random_seed already defined");

  g_random_seed = new ::mare::internal::random_seed();
}
/** @endcond HIDDEN */

}; // namespace runtime

/** @cond HIDDEN */
/**
   This namespace includes methods and classes that are not
   yet considered part of the main MARE API. Use them at your own
   risk. They are not as well tested as the rest of the API, and
   some of them may never be part of the main MARE API.
*/
namespace experimental {
}//namespace experimental
/** @endcond HIDDEN */


}; // namespace mare


