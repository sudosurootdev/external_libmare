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
using namespace std;

static atomic<int> task_running(0);
static atomic<int> keep_blocking(1);

int main() {

  // Initialize the MARE runtime.
  runtime::init();

  // Create attrs object
  auto attrs = create_task_attrs(attr::blocking);

  // Create blocking task with two lambdas.  The first lambda is
  // executed as soon as the t gets scheduled. The second one
  // is executed when t is canceled, so that t stops blocking.
  auto t = create_task(with_attrs(attrs, [] {
        task_running = 1;
        while(keep_blocking){}
      },
      []() mutable {
        printf("Task canceled.  Stop blocking...\n");
        keep_blocking = 0;
      }
      ));

  // Launch long-running task
  launch(t);

  while(!task_running){};
  // Sleep for a while
  usleep(10000);
  cancel(t);

  // Wait for long-running task to finish
  wait_for(t);

  // Shutdown the MARE runtime.
  runtime::shutdown();

  return 0;
}
