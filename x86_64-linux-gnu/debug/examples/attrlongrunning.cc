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

int main() {

  // Initialize the MARE runtime.
  runtime::init();

  // Create attrs object
  auto attrs = create_task_attrs(mare::internal::attr::long_running);

  // Create long-running task
  auto t = create_task(with_attrs(attrs,[]{
        printf("Doing a lot of work...\n");
      }));

  // Launch long-running task
  launch(t);

  // Wait for long-running task to finish
  wait_for(t);

  // Shutdown the MARE runtime.
  runtime::shutdown();

  return 0;
}
