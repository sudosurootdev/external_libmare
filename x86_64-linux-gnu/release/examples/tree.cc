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
#include <math.h>

#include "tree.hh"
#include "assembly-kernel.hh"

using namespace std;

size_t Node::s_ids = 0;

void Tree::showSubTree(Node* node)
{
  if (!node)
    return;

  if (node->firstChild())
    MARE_ALOG("Node %zu: children: %zu - %zu",
          node->id(), node->firstChild()->id(),
          node->secondChild()->id());

  showSubTree(node->firstChild());
  showSubTree(node->secondChild());
}

void Tree::checkStyled(Node* node)
{
  MARE_INTERNAL_ASSERT(node->_styled, "");
  MARE_INTERNAL_ASSERT(node->_matched, "");

  if (node->firstChild())
    checkStyled(node->firstChild());
  if (node->secondChild())
    checkStyled(node->secondChild());
}

void Tree::buildSubTree(Node* node, size_t height)
{
  if (height == 0)
    return;

  node->_childA = new Node();
  node->_childB = new Node();
  node->_childA->_sibling = node->_childB;

  node->_childA->_parent = node;
  node->_childB->_parent = node;

  buildSubTree(node->_childA, height-1);
  buildSubTree(node->_childB, height-1);
}

static const long long int LONG_WASTE = 4;
static const long long int SHORT_WASTE = 1;

void Node::wasteTime()
{
  size_t max = SHORT_WASTE;

  if (!isMatched())
    max = LONG_WASTE;

  for (size_t i = 0; i < max; ++i){
    for( size_t j = 0; j < max; ++j){
      kernel();
    }
  }
}
