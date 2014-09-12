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

#include <mare/runtime.hh>

namespace mare
{

namespace internal
{

class dealer {
public:
  dealer(size_t start, size_t cards, size_t skip) :
    _rgen(MARE_RANDOM_SEED()),
    _deck(cards)
  {
    // make an initial deck
    for (size_t i = 0, idx = start; i < _deck.size(); idx++) {
      if (idx == skip)
        continue;
      _deck[i] = idx;
      ++i;
    }
  }
  MARE_DEFAULT_METHOD(~dealer());

  void shuffle() {
    size_t top = _deck.size();
    while (top > 1) {
      size_t r = _rgen() % top;
      --top;
      auto t = _deck[top];
      _deck[top] = _deck[r];
      _deck[r] = t;
    }
  }

  std::vector<size_t>& get_hand() {
    return _deck;
  }

private:
  std::default_random_engine _rgen;
  std::vector<size_t> _deck;
};

} //namespace internal

} //namespace mare
