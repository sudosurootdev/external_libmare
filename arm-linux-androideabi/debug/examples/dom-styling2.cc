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

#include "tree.hh"
#include "tree.cc"

using namespace std;

mare::group_ptr g_group = nullptr;

// This algorithm styles a DOM tree. This sample code is based on the
// CSS subsystem of the ZOOMM parallel browser.
//
// For each node in the DOM tree, we spawn two tasks:
//
// - matching task: finds the rules that apply to a DOM node and stores
// them in a data structure
//
// - styling task: traverses the data structure populated by the matching
// task working on the same node and applies rules to DOM node.
//
// There are two dependences the algorithm must maintain. First,
// a styling task can only execute after the matching task working on
// the same node has completed execution.
//
// And second, a styling task for a node N can only execute after the styling
// task for N's parent has completed.
//
// Notice that, while matching tasks are embarrasingly parallel, styling
// tasks must obey dependences. In practice, this means that styling tasks
// for sibling nodes execute in parallel.

static void match(Node* node, mare::task_ptr parent_styling_task);
static void style(Node* node);

// Styles node. The styline code has been substituted by a LOG message
// and a sleep.
static void style(Node* node)
{
  node->wasteTime();
  node->markAsStyled();
}

// Matches node. Mathing Tasks are embarrasingly parallel.
// The matching code has been substituted by a LOG message
// and a sleep
static void match(Node* node, mare::task_ptr styling_task_parent)
{
  Node* child = node->firstChild();

  while (child)
  {
    //Create styling and matching tasks for child node
    auto styling_task = mare::create_task([child] {
        style(child);
      });
    auto matching_task = mare::create_task([child, styling_task] {
        match(child, styling_task);
      });

    // Make sure that the matching task executes before the styling task
    matching_task >> styling_task;
    styling_task_parent >> styling_task;

    // Dependences are ready, launch tasks
    mare::launch(g_group, matching_task);
    mare::launch(g_group, styling_task);

    child = child->nextSibling();
  }

  node->wasteTime();
  node->markAsMatched();
}

int main ()
{
  mare::runtime::init();
  g_group = mare::create_group("matching and styling task group");

  Tree* tree = new Tree(8);
  MARE_ALOG("Created tree with %zu nodes", tree->numNodes());

  auto styling_task = mare::create_task([tree]{
      style(tree->root());
    });

  auto matching_task = mare::create_task([tree, styling_task]{
      match(tree->root(), styling_task);
    });

  matching_task >> styling_task;

  mare::launch_and_reset(g_group, matching_task);
  mare::launch_and_reset(g_group, styling_task);

  mare::wait_for(g_group);

  tree->checkStyled();
  MARE_ALOG("Tree has been styled");

  mare::runtime::shutdown();
  return 0;
}
