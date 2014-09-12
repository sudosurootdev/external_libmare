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

#include <atomic>
#include <mare/mare.h>

using namespace std;

// Counts the number of tasks that execute before the group gets
// canceled
atomic<size_t> counter;

int main() {

  // Initialize the MARE runtime.
  mare::runtime::init();
  auto group = mare::create_group();

  // Create 2000 tasks that print Hello World!.
  for (int i = 0; i < 2000; i++){
    mare::launch(group, [] {
        counter++;
        usleep(7);
      });
  }

  // Cancel group
  mare::cancel(group);

  // Wait for group to cancel
  mare::wait_for(group);
  MARE_ALOG("wait_for returned after %zu tasks executed", counter.load());

  // Shutdown the MARE runtime.
  mare::runtime::shutdown();

  return 0;
}

