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
/** @file interval_set.hh */
#pragma once

#ifdef HAVE_OPENCL

#include <iterator>
#include <set>

#include <mare/internal/debug.hh>
#include <mare/internal/interval.hh>

namespace mare {
namespace internal {

struct interval_set_base {};

MARE_GCC_IGNORE_BEGIN("-Weffc++");

/**
   An interval set stores (non-overlapping) intervals.  Intervals can
   be looked up uniquely through any of their contained points.
*/
template<class T, template<class> class Interval = interval>
class interval_set : public interval_set_base {
private:
  using set_type = typename std::set<Interval<T>>;
  set_type _set;

public:
  using interval_type = Interval<T>;
  using element_type = T;
  using iterator = typename set_type::iterator;
  using const_iterator = typename set_type::const_iterator;

  interval_set() : _set() {}

  /** @returns iterator to the end of the set */
  iterator end() {
    return _set.end();
  }

  /** @returns const iterator to the end of the set */
  const_iterator end() const {
    return _set.end();
  }

  /** @returns const iterator to the end of the set */
  const_iterator cend() const {
    return end();
  }

  /** @returns iterator to the start of the set */
  iterator begin() {
    return _set.begin();
  }

  /** @returns const iterator to the start of the set */
  const_iterator begin() const {
    return _set.begin();
  }

  /** @returns const iterator to the start of the set */
  const_iterator cbegin() const {
    return begin();
  }

public:
  /**
     @returns iterator to the first interval that contains point, or
              end(), if none is found.

     @param point point that is contained in any interval to be looked
            up.
  */
  iterator find(T const& point) {
    iterator it = _set.lower_bound(interval_type(point, point));
    if (it != end()) {
      if (it->begin() <= point && point < it->end())
        return it;
    }

    if (it == begin()) {
      // empty set -> return nothing
      return end();
    }

    // try previous interval
    --it;
    MARE_INTERNAL_ASSERT(it->begin() <= point, "invalid lower bound");
    if (point < it->end())
      return it;

    return end();
  }

  /**
     @returns const iterator to the first interval that contains
              point, or end(), if none is found.

     @param point point that is contained in any interval to be looked
            up.
  */
  const_iterator find(T const& point) const {
    const_iterator it = _set.lower_bound(interval_type(point, point));
    if (it != end()) {
      if (it->begin() <= point && point < it->end())
        return it;
    }

    if (it == begin()) {
      // empty set -> return nothing
      return end();
    }

    // try previous interval
    --it;
    MARE_INTERNAL_ASSERT(it->begin() <= point, "invalid lower bound");
    if (point < it->end())
      return it;

    return end();
  }

  /**
     Inserts an interval into the set.

     @param interval interval to be inserted
  */
  void insert(interval_type const& interval) {
    _set.insert(interval);
  }

  /**
     Removes interval from set.

     @returns number of intervals removed

     @param interval interval to be removed
  */
  size_t erase(interval_type const&& interval) {
    return _set.erase(std::forward<interval_type const>(interval));
  }

};

MARE_GCC_IGNORE_END("-Weffc++");

}; // namespace internal
}; // namespace mare

#endif // HAVE_OPENCL
