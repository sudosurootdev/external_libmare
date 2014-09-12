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

#include <array>
#include <vector>

#include <mare/exceptions.hh>

#include <mare/internal/debug.hh>
#include <mare/internal/index.hh>

namespace mare {

namespace internal {

template<size_t DIMS>
bool in_bounds(const std::array<size_t, DIMS>& b,
               const std::array<size_t, DIMS>& e,
               const std::array<size_t, DIMS>& it)
{
  for(size_t i = 0; i < DIMS; i++) {
    if(!((b[i] <= it[i]) && (it[i] < e[i]))) {
      return false;
    }
  }
  return true;
}

template<size_t DIMS>
struct check_bounds;

template<>
struct check_bounds<1>
{
  static void check(const std::array<size_t, 1>& b,
                    const std::array<size_t, 1>& e,
                    const std::array<size_t, 1>& it) {
    bool flag = in_bounds(b, e, it);
    MARE_API_ASSERT(flag,
                    "it: [%zu], lb: [%zu], ub: [%zu]",
                    it[0], b[0], e[0]);
  }
};

template<>
struct check_bounds<2>
{
  static void check(const std::array<size_t, 2>& b,
                    const std::array<size_t, 2>& e,
                    const std::array<size_t, 2>& it) {
    bool flag = in_bounds(b, e, it);
    MARE_API_ASSERT(flag,
                    "it: [%zu, %zu], lb: [%zu, %zu], ub: [%zu, %zu]",
                    it[0], it[1], b[0], b[1], e[0], e[1]);
  }
};

template<>
struct check_bounds<3>
{
  static void check(const std::array<size_t, 3>& b,
                    const std::array<size_t, 3>& e,
                    const std::array<size_t, 3>& it) {
    bool flag = in_bounds(b, e, it);
    MARE_API_ASSERT(flag,
                    "it: [%zu, %zu, %zu], lb: [%zu, %zu, %zu],"
                    "ub: [%zu, %zu, %zu]", it[0], it[1], it[2],
                    b[0], b[1], b[2], e[0], e[1], e[2]);
  }
};

template<size_t DIMS>
class range_base
{
protected:
  std::array<size_t, DIMS>  _b;
  std::array<size_t, DIMS>  _e;
  const static size_t s_dims = DIMS;
public:
  range_base() : _b(), _e() { }

  range_base(const std::array<size_t, DIMS>& bb,
             const std::array<size_t, DIMS>& ee) : _b(bb), _e(ee) { }

  const size_t& begin(const size_t i) const {
    MARE_API_ASSERT(i < DIMS, "Index out of bounds");
    return _b[i];
  }

  const size_t& end(const size_t i) const {
    MARE_API_ASSERT(i < DIMS, "Index out of bounds");
    return _e[i];
  }

  inline size_t num_elems(size_t i) const {
    MARE_API_ASSERT(_e[i] - _b[i], "begin == end");
    return (_e[i] - _b[i]);
  }

  const std::array<size_t, DIMS>& begin() const { return _b; }

  const std::array<size_t, DIMS>& end() const { return _e; }

  const size_t& dims() const { return s_dims; }
};

template<size_t DIMS>
class range;

// squelch GCC complaints about non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");
template<>
class range<1> : public range_base<1>
{
MARE_GCC_IGNORE_END("-Weffc++");
public:
  range() : range_base<1>() { }

  range(const std::array<size_t, 1>& bb,
        const std::array<size_t, 1>& ee) : range_base<1>(bb, ee) { }

  range(size_t b0, size_t e0) {
    MARE_API_ASSERT(b0 < e0, "Invalid range parameters");
    _b[0] = b0;
    _e[0] = e0;
  }

  explicit range(size_t e0) : range(0, e0) { }

  inline size_t size() const {
    return num_elems(0);
  }

  void increment_index(mare::internal::index<1>& it) {
    check_bounds<1>::check(_b, _e, it.data());
    it[0] += 1;
  }

  size_t index_to_linear(const mare::internal::index<1>& it) const {
    check_bounds<1>::check(_b, _e, it.data());
    return (it[0] - _b[0]);
  }

  mare::internal::index<1> linear_to_index(size_t idx) const {
    index<1> it(idx + _b[0]);
    check_bounds<1>::check(_b, _e, it.data());
    return it;
  }

  void print() const {
    MARE_ALOG("begin[%zu], end[%zu]", _b[0], _e[0]);
  }
};

// squelch GCC complaints about non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");
template<>
class range<2> : public range_base<2>
{
MARE_GCC_IGNORE_END("-Weffc++");
public:
  range() : range_base<2>() { }

  range(const std::array<size_t, 2>& bb,
        const std::array<size_t, 2>& ee) : range_base<2>(bb, ee) { }

  range(size_t b0, size_t e0, size_t b1, size_t e1) {
    MARE_API_ASSERT(((b0 < e0) && (b1 < e1)),
                      "Invalid range parameters");
    _b[0] = b0;
    _b[1] = b1;
    _e[0] = e0;
    _e[1] = e1;
  }

  range(size_t e0, size_t e1) :
    range(0, e0, 0, e1) { }

  void increment_index(mare::internal::index<2>& it) {
    check_bounds<2>::check(_b, _e, it.data());
    if((it[0] + 1) < _e[0]) {
      it[0] += 1;
      return;
    }

    if((it[1] + 1) < _e[1]) {
      it[0] = _b[0];
      it[1] += 1;
      return;
    }
    it[0] += 1;
    it[1] += 1;
  }

  inline size_t size() const {
    return num_elems(0) * num_elems(1);
  }

  size_t index_to_linear(const mare::internal::index<2>& it) const {
    check_bounds<2>::check(_b, _e, it.data());
    return (it[1] - _b[1]) * num_elems(0) + (it[0] - _b[0]);

  }

  mare::internal::index<2> linear_to_index(size_t idx) const {
    size_t j = idx / num_elems(0);
    size_t i = idx % num_elems(0);
    index<2> it(i + _b[0], j + _b[1]);

    check_bounds<2>::check(_b, _e, it.data());
    return it;
  }

  void print() const {
    MARE_ALOG("begin[%zu, %zu], end[%zu, %zu]", _b[0], _b[1], _e[0], _e[1]);
  }
};

// squelch GCC complaints about non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");
template<>
class range<3> : public range_base<3>
{
MARE_GCC_IGNORE_END("-Weffc++");
public:
  range() : range_base<3>() { }

  range(const std::array<size_t, 3>& bb,
        const std::array<size_t, 3>& ee) : range_base<3>(bb, ee) { }

  range(size_t b0, size_t e0, size_t b1, size_t e1, size_t b2, size_t e2) {
    MARE_API_ASSERT(((b0 < e0) && (b1 < e1) && (b2 < e2)),
                       "Invalid range parameters");
    _b[0] = b0;
    _b[1] = b1;
    _b[2] = b2;
    _e[0] = e0;
    _e[1] = e1;
    _e[2] = e2;
  }

  range(size_t e0, size_t e1, size_t e2) :
    range(0, e0, 0, e1, 0, e2) { }

  inline size_t size() const {
    return num_elems(0) * num_elems(1) * num_elems(2);
  }

  void increment_index(mare::internal::index<3>& it) {
    check_bounds<3>::check(_b, _e, it.data());
    if((it[0] + 1) < _e[0]) {
      it[0] += 1;
      return;
    }

    if((it[1] + 1) < _e[1]) {
      it[0] = _b[0];
      it[1] += 1;
      return;
    }

    if((it[2] + 1) < _e[2]) {
      it[0] = _b[0];
      it[1] = _b[1];
      it[2] += 1;
      return;
    }
    it[0] += 1;
    it[1] += 1;
    it[2] += 1;
  }

  size_t index_to_linear(const mare::internal::index<3>& it) const {
    check_bounds<3>::check(_b, _e, it.data());
    return ((it[2] - _b[2]) * num_elems(0) * num_elems(1) +
            (it[1] - _b[1]) * num_elems(0) + (it[0] - _b[0]));
  }

  mare::internal::index<3> linear_to_index(size_t idx) const {
    size_t k = idx / (num_elems(0) * num_elems(1));
    size_t j = (idx / num_elems(0)) % num_elems(1);
    size_t i = idx % num_elems(0);
    index<3> it(i + _b[0], j + _b[1], k + _b[2]);

    check_bounds<3>::check(_b, _e, it.data());
    return it;
  }

  void print() const {
    MARE_ALOG("begin[%zu, %zu, %zu], end[%zu, %zu, %zu]", _b[0], _b[1], _b[2],
              _e[0], _e[1], _e[2]);
  }
};
} // ns internal

} // ns mare
