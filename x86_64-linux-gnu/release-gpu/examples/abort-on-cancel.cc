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

#include <mare/mare.h>

using namespace mare;

int main() {

  // Initialize the MARE runtime.
  mare::runtime::init();

  // Create task
  auto t = mare::create_task([]{
      size_t num_iters = 0;
      while(1)
      {
        // Check whether the task needs to stop execution.
        // Without abort_on_cancel() the task would never
        // return
        mare::abort_on_cancel();

        MARE_ALOG("Task has executed %zu iterations!", num_iters);
        usleep(100);
        num_iters++;
      }
    });

  // Create group g
  auto g = mare::create_group("example group");

  // Launch t into g. We don't use t after launch(), so we can
  // use launch_and_reset
  mare::launch_and_reset(g, t);

  // Wait for the task to execute a few iterations
  usleep(10000);

  // Cancel group g, and wait for t to complete
  mare::cancel(g);
  mare::wait_for(g);

  // Shutdown the MARE runtime.
  mare::runtime::shutdown();

  return 0;
}

