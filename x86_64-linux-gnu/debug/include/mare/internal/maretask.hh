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

#include <forward_list>
#include <functional>
#include <memory>
#include <iostream>
#include <typeinfo>
#include <type_traits>
#include <typeindex>
#include <utility>

#include <mare/common.hh>
#include <mare/internal/functiontraits.hh>
#include <mare/internal/macros.hh>
#include <mare/internal/task.hh>

// We disable -Weffc++ for this file because the template metaprogramming here
// involves a great deal of subclassing of classes with pointer members. This
// causes -Weffc++ to emit huge numbers of warnings requesting that we write
// copy constructors and assignment operators; however, in these cases the
// defaults are sufficient, and manually defaulting all of the implementations
// will harm code readability greatly for no benefit.
MARE_GCC_IGNORE_BEGIN("-Weffc++");

// This code is most easily comprehended if read from bottom to top.

namespace mare { namespace internal {

class group;

#if 0
//def MARE_USE_FTRACE_LOGGER
template<typename F>
struct is_function_ptr : std::integral_constant< bool,
  function_traits<F>::kind == function_kind::function_pointer> {};

template<typename F>
struct is_lambda_or_member_function : std::integral_constant< bool,
  function_traits<F>::kind == function_kind::mutable_lambda_or_member ||
      function_traits<F>::kind == function_kind::const_lambda_or_member> {};

template<class F>
struct task_id_gen {
    typedef size_t type_id;
    template <typename U = F>
      static
      type_id get_id(F& f,
          const typename std::enable_if<is_function_ptr<U>::value,
          type_id>::type = 0) {
        return reinterpret_cast<type_id>(f);
      }

    // hash code will return the same value for all instances of the same
    // lambda, but also may return the same value for different lambdas
    template <typename U = F>
      static
      type_id get_id(F&,
          const typename std::enable_if<!is_function_ptr<U>::value,
          type_id>::type = 0) {
        return std::type_index(typeid(F)).hash_code();
      }
};
#endif

/// Implements tasks which take no arguments.
template <typename F>
struct mare_task_nullary : public task {

  typedef typename function_traits<F>::f_type_in_task f_type;
  typedef typename has_cancel_notify<F>::type notify_support;

  mare_task_nullary(F&& f, group *g, task_attrs a)
    : task(g, a)
    , _f(f)
  { }

  virtual ~mare_task_nullary() = 0;

  virtual void execute() {
    _f();
  }

  virtual void cancel_notify(){
    do_cancel_notify(notify_support());
  }

  virtual uintptr_t get_source() const {
    return reinterpret_cast<uintptr_t>(&typeid(_f));
  }

private:

  void do_cancel_notify(std::true_type) {
    _f.cancel_notify();
  }

  void do_cancel_notify(std::false_type) {
    MARE_FATAL("Task has no cancel_notify() method.");
  }

  f_type _f;
};

template <typename F>
mare_task_nullary<F>::~mare_task_nullary() { };

/// Dispatches to the correct implementation for a task of a given arity.
template <typename F, size_t Arity>
struct mare_task_arity_dispatch {
  mare_task_arity_dispatch(F&&, group*, task_attrs ) {
    static_assert(Arity == 0, "Tasks may not take any parameters.");
  }
};

template <typename F>
struct mare_task_arity_dispatch<F, 0> : public mare_task_nullary<F> {
  mare_task_arity_dispatch(F&& f, group *g, task_attrs a) :
    mare_task_nullary<F>(std::forward<F>(f), g, a) { }
};

/// Represents a low-level mare task.
template <typename F>
struct mare_task :
    public mare_task_arity_dispatch<F, function_traits<F>::arity> {
  mare_task(F&& f, group *g, task_attrs a) :
    mare_task_arity_dispatch<F, function_traits<F>::arity>(
        std::forward<F>(f), g, a) { }

  // Forbid copying and assignment.
  MARE_DELETE_METHOD(mare_task(mare_task<F> const& other));
  MARE_DELETE_METHOD(mare_task<F>& operator=(mare_task<F> const& other));
};

/// Represents a low-level mare task with a cancel_notify method.
template <typename F, typename N>
struct mare_task_with_notify :
    public mare_task_arity_dispatch<F, function_traits<F>::arity> {

  typedef typename function_traits<N>::f_type_in_task f_notify;

  mare_task_with_notify(F&& f, N&& n, group *g, task_attrs a) :
    mare_task_arity_dispatch<F, function_traits<F>::arity>(std::forward<F>(f),
                                                           g, a),
    _notify(n) {

    static_assert(function_traits<N>::arity == 0,
                  "Cancel handler can't have arguments.");
    static_assert(std::is_same<typename function_traits<N>::return_type,
                  void>::value,
                  "The return type of a cancel_notify function must be void");
  }

  // Forbid copying and assignment.
  MARE_DELETE_METHOD(mare_task_with_notify(mare_task_with_notify<F, N>
                                           const& other));
  MARE_DELETE_METHOD(mare_task_with_notify<F, N>& operator=
                     (mare_task_with_notify<F, N> const& other));

  virtual void cancel_notify() {
    _notify();
  }

private:
  f_notify _notify;
};

MARE_GCC_IGNORE_END("-Weffc++");

} } // namespace mare::internal
