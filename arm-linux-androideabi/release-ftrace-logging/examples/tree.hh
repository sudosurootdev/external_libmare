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
#pragma once

#include <atomic>
#include <mare/internal/debug.hh>

class Tree;

/** Binary tree */
class Node {

public:

  size_t id() const {return _id;}

  Node* firstChild() const {return _childA;}
  Node* secondChild() const {return _childB;}

  Node* nextSibling() const {return _sibling;}

  void markAsMatched() {
    MARE_INTERNAL_ASSERT(!_matched, "");
    _matched = true;
  }

  void markAsStyled() {
    MARE_INTERNAL_ASSERT(!_styled, "");
    MARE_INTERNAL_ASSERT(_parent?_parent->_styled.load():true, "");
    MARE_INTERNAL_ASSERT(_matched, "");

    _styled = true;
  }

  bool isMatched() {
    return _matched.load();
  }

  bool isStyled() {
    return _matched.load();
  }

  void wasteTime();

protected:

  Node():
    _childA(nullptr),
    _childB(nullptr),
    _parent(nullptr),
    _sibling(nullptr),
    _matched(false),
    _styled(false),
    _foundPrime(false),
    _id(++s_ids)
  {}

  Node* _childA;
  Node* _childB;
  Node* _parent; //we use this for asserts
  Node* _sibling;
  std::atomic<bool> _matched;
  std::atomic<bool> _styled;
  std::atomic<bool> _foundPrime;
  const size_t _id;
  static size_t s_ids;
  friend class Tree;
};

class Tree {

public:

  Tree (size_t height)
    :_root(new Node()) {
    buildSubTree(_root, height-1);
  }

  size_t numNodes() const {
    return Node::s_ids;
  }

  Node* root() const {
    return _root;
  }

  void checkStyled() {
    checkStyled(_root);
  }

  void show() {
    showSubTree(_root);
  }

private:

  void showSubTree(Node* node);
  void checkStyled(Node* node);
  void buildSubTree(Node* node, size_t height);

  Node* _root;
};
