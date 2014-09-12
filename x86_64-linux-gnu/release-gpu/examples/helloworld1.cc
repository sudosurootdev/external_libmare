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

using namespace std;

int main() {

  // Initialize the MARE runtime.
  mare::runtime::init();

  // Create task that prints "Hello "
  auto hello = mare::create_task([]{
      printf("Hello ");
    });

  // Create task that prints " World!"
  auto world = mare::create_task([]{
      printf("World!\n");
    });

  // Make sure that "World!" prints after "Hello"
  mare::after(hello, world);

  // Launch both tasks
  mare::launch(hello);
  mare::launch(world);

  // Wait for world to complete
  mare::wait_for(world);

  // Shutdown the MARE runtime.
  mare::runtime::shutdown();

  return 0;
}
