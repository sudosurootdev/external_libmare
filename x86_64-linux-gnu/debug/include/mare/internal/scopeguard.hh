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

namespace mare {
namespace internal {

/// \brief ensures execution of an action at the end of a scope
///
/// The effect of defining a scope guard is similar to Java's
/// `try ... finally` statement.
///
/// \par Example
/// \code
/// {
///   scope_guard guard([] { std::cout << "hi" << std::endl; });
///   throw std::exception("die");
/// } // still prints "hi"
/// \endcode
template <typename NullaryFn>
struct scope_guard {
  scope_guard(NullaryFn&& fn)
    : _active(true),
      _fn(std::move(fn)) {}

  scope_guard(scope_guard&& other)
    : _active(other._active),
      _fn(std::move(other._fn)) {
    other.reset();
  }

  MARE_DELETE_METHOD(scope_guard());
  MARE_DELETE_METHOD(scope_guard(const scope_guard&));
  MARE_DELETE_METHOD(scope_guard& operator=(scope_guard const&));

  /// \brief prohibits scope guard from executing
  void reset() {
    _active = false;
  }

  ~scope_guard() {
    if (_active)
      _fn();
  }

private:
  bool _active;
  NullaryFn const _fn;
};

template <typename NullaryFn>
inline scope_guard<NullaryFn> make_scope_guard(NullaryFn&& fn)
{
  return scope_guard<NullaryFn>(std::move(fn));
}

}; // namespace internal
}; // namespace mare
