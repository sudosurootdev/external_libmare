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
/** @file cofstack.hh */
#pragma once

#include <mare/cofdeque.hh>

namespace mare {

/** @addtogroup cof_stack
@{ */
/**
   LILO stack implemention built on top of cofdeque.hh

   @note1 As it is built on top of MARE's DQueue, similar to that
   structure, the size is bounded at creation time.
*/

template<class T, class PREDICTOR = internal::cof::default_predictor>
class cof_stack {
public:
  typedef cof_deque<T, PREDICTOR> container_type;
  typedef T value_type;

  explicit cof_stack(size_t qsize) : _c(qsize) {}
  explicit cof_stack(cof_stack<T>& rhs) { *this = rhs; }

  /** @sa cof_deque::right_push */
  bool push(const value_type& v) { return _c.right_push(v); }

  /** @sa cof_deque::right_pop */
  bool pop(value_type& r) { return _c.right_pop(r); }

  /** @sa cof_deque::size */
  size_t size() const { return _c.size(); }

private:
  container_type _c;
};

} // namespace mare

/** @} */ /* end_addtogroup cof_stack */
