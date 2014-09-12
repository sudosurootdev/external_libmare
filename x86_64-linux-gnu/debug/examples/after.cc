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
#include <stdio.h>

using namespace mare;

int main() {

  // Initialize the MARE runtime.
  mare::runtime::init();

  // Create group.
  auto g = mare::create_group("Hello World Group");

  // Create tasks t1 and t2
  auto t1 = mare::create_task([]{
      printf ("Hello World! from task t1\n");
    });

  auto t2 = mare::create_task([]{
      printf ("Hello World! from task t2\n");
    });

  // Create dependency between t1 and t2
  mare::after(t1, t2);

  // Launch both t1 and t2 into g
  mare::launch(g, t1);
  mare::launch(g, t2);

  // Wait until t1 and t2 finish.
  mare::wait_for(g);

  // Shutdown the MARE runtime.
  mare::runtime::shutdown();

  return 0;
}
