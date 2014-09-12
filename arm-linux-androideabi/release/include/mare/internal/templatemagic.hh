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

namespace mare
{

namespace internal
{

// Counts the number of types
template <typename... TS>
struct count_typenames;

template<>
struct count_typenames<> {
  enum {value = 0 };
};

template<typename T, typename... TS>
struct count_typenames<T, TS...> {
  enum {value = 1 + count_typenames<TS...>::value};
};

// Checks whether a type exists
template <typename T, typename... TS>
struct type_exists;

template<typename T>
struct type_exists<T> {
  enum {value = 0};
};

template<typename T, typename T2, typename... TS>
struct type_exists<T, T2, TS...> {
  enum {
    value = ((std::is_same<T, T2>::value == 1) ?
             1 : type_exists<T, TS...>::value) };
};

// Checks whether there are duplicated types
template <typename... TS>
struct duplicated_types;

template<>
struct duplicated_types<> {
  enum {value = 0};
};

template <typename T, typename... TS>
struct duplicated_types<T, TS...> {
  enum {
    value = ((type_exists<T, TS...>::value == 1) ?
             1 : duplicated_types<TS...>::value) };
};

// Checks whether T has a method called cancel_notify
template<typename T>
class has_cancel_notify {
private:
  template <typename U>
  static auto check(int) ->
    typename std::enable_if
                 <std::is_same<decltype(std::declval<U>
                                        ().cancel_notify(), int()),int>::value,
    std::true_type>;
  template <typename>
  static std::false_type check(...);

  typedef decltype(check<T>(0)) enable_if_type;

public:

  typedef typename enable_if_type::type type;

  enum{
    value = type::value
  };

}; //has_cancel_notify

/** Type is the larger of the two*/
template<typename A, typename B>
struct larger_type {
  typedef typename std::conditional<(sizeof(A) > sizeof(B)), A, B>::type type;
};

} //namespace internal
} //namespace mare

