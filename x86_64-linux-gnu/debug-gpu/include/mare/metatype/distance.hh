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

#include <type_traits>

#include <mare/internal/compat.h>

namespace mare {

namespace internal {

template<class InputIterator,
         bool = std::is_integral<InputIterator>::value>
class distance_helper
{
public:
  typedef typename std::iterator_traits<InputIterator>::difference_type
    _result_type;

  static _result_type s_distance(InputIterator first, InputIterator last)
  {
    return std::distance(first, last);
  }
};

template <typename IntegralType>
class distance_helper<IntegralType,true>
{
public:
  typedef IntegralType _result_type;

  static _result_type s_distance(IntegralType first, IntegralType last)
  {
    return last - first;
  }
};

template<class InputIterator>
auto distance(InputIterator first, InputIterator last) ->
  typename distance_helper<InputIterator>::_result_type
{
  return distance_helper<InputIterator>::s_distance(first, last);
}
} // namespace internal

} // namespace mare
