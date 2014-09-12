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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <mare/mare.h>
#include <mare/schedulerstorage.hh>

using namespace std;
using namespace mare;

struct somestate {
  size_t counter;
};

const scheduler_storage_ptr<somestate> s_sls_state;

int
main()
{
  // Initialize the MARE runtime.
  runtime::init();

  auto g = create_group("test");

  for (size_t i = 0; i < 1000; ++i) {
    launch(g, [i] {
        size_t c = ++s_sls_state->counter;
        // values for c are consecutive on a per-scheduler basis,
        // i.e., each MARE schedulers creates its own local copy of
        // the object.  Access to the object is concurrency-safe
        // without any synchronization (e.g., mutexes).
        printf("%p: %8zu\n", s_sls_state.get(), c);
      });
  }

  wait_for(g);

  // Shutdown the MARE runtime.
  runtime::shutdown();

  return 0;
}

