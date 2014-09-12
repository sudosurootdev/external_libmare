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

namespace mare { namespace internal {

struct function_kind {
  enum {
    mutable_lambda_or_member,
    const_lambda_or_member,
    function_pointer,
    invalid_function = 42,
  };
};

// Matches lambdas and member functions.
template <typename F>
struct function_traits_lambda_or_member;

// Matches C::operator(A...) and [](A...) mutable { }
template <typename FReturnType, typename FClassType, typename... FArgs>
struct function_traits_lambda_or_member<FReturnType(FClassType::*)(FArgs...)> {
  typedef FReturnType return_type;

  enum { arity = sizeof...(FArgs) };
  enum { is_const = false };
  enum { kind = function_kind::mutable_lambda_or_member };
};

// Matches const C::operator(A...) and [](A...) { }
template <typename FReturnType, typename FClassType, typename... FArgs>
struct function_traits_lambda_or_member<FReturnType(FClassType::*)
                                        (FArgs...) const>
{
  typedef FReturnType return_type;

  enum { arity = sizeof...(FArgs) };
  enum { is_const = true };
  enum { kind = function_kind::const_lambda_or_member };
};

// This struct is needed because of VS2012. Neither GCC or CLANG need it
template<typename>
struct function_traits_lambda_or_member {
  typedef void return_type;

  enum { arity = -1 };
  enum { is_const = false };
  enum { kind = function_kind::invalid_function };
};

// Computes properties of a lambda or function : its arity,
// its return type and the types of its arguments.
//
// We need to use std::remove_references because forwarding causes
// some drama with references. By removing the references, the
// function_traits_lambda_or_member templates can accurately match F.
template <typename F> struct function_traits;

// Matches lambdas and member functions (both mutable and const)
template <typename F>
struct function_traits {

  typedef typename std::remove_reference<F>::type f_type_in_task;
  typedef decltype(&f_type_in_task::operator()) pointer_type;

  typedef typename
    function_traits_lambda_or_member<pointer_type>::return_type return_type;

  enum { arity = function_traits_lambda_or_member<pointer_type>::arity };
  enum { is_const = function_traits_lambda_or_member<pointer_type>::is_const };
  enum { kind = function_traits_lambda_or_member<pointer_type>::kind };
};

// Matches functions
template <typename FReturnType, typename... FArgs>
struct function_traits<FReturnType(&)(FArgs...)> {

  typedef FReturnType(*pointer_type)(FArgs...);
  typedef FReturnType(*f_type_in_task)(FArgs...);
  typedef FReturnType return_type;

  enum { arity = sizeof...(FArgs) };
  enum { is_const = true };
  enum { kind = function_kind::function_pointer };
};

} } // namespace mare::internal

