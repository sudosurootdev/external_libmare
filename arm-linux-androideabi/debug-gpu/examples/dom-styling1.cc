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
// CSS subsystem of the ZOOMM parallel browser. For each node in the
// DOM tree, we spawn one task. The only restriction that we impose in
// this version of the algorithm is that a node must be styled before
// its children can be styled.

static void styleNode(Node* node)
{
  node->wasteTime();
  node->markAsMatched();
  node->markAsStyled();

  if (node->firstChild()) {
    mare::launch(g_group, [node]() { styleNode(node->firstChild());} );
  }
  if (node->firstChild()) {
    mare::launch(g_group, [node]() { styleNode(node->secondChild());});
  }
}

int main ()
{
  mare::runtime::init();
  g_group = mare::create_group("ovals task group");

  Tree* tree = new Tree(8);
  MARE_ALOG("Created tree with %zu nodes", tree->numNodes());

  mare::launch(g_group, [tree]() {styleNode(tree->root());});
  mare::wait_for(g_group);

  mare::runtime::shutdown();
  return 0;
}
