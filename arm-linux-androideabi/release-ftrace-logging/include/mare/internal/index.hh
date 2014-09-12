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

#include <mare/exceptions.hh>

namespace mare {

namespace internal {

//forward declaration
template<size_t DIMS>
class index;

template<size_t DIMS>
class index_base
{
protected:
  std::array<size_t, DIMS> _data;
public:
  index_base() : _data() { _data.fill(0); }

  index_base(const std::array<size_t, DIMS>& rhs) : _data(rhs) { }

  size_t& operator [] (size_t i) { return _data[i]; }

  const size_t& operator [] (size_t i) const { return _data[i]; }

  virtual ~index_base() { }

  const std::array<size_t, DIMS>& data() const { return _data; }

  bool operator == (const index_base<DIMS>& rhs) const {
    return (const_cast<std::array<size_t, DIMS>&>(_data) == rhs.data());
  }

  bool operator != (const index_base<DIMS>& rhs) const {
    return (const_cast<std::array<size_t, DIMS>&>(_data) == rhs.data());
  }

  bool operator < (const index_base<DIMS>& rhs) const {
    return (const_cast<std::array<size_t, DIMS>&>(_data) < rhs.data());
  }

  bool operator <= (const index_base<DIMS>& rhs) const {
    return (const_cast<std::array<size_t, DIMS>&>(_data) <= rhs.data());
  }

  bool operator > (const index_base<DIMS>& rhs) const {
    return (const_cast<std::array<size_t, DIMS>&>(_data) > rhs.data());
  }

  bool operator >= (const index_base<DIMS>& rhs) const {
    return (const_cast<std::array<size_t, DIMS>&>(_data) >= rhs.data());
  }

  index<DIMS>& operator += (const index<DIMS>& rhs) {
    for(size_t i = 0; i < DIMS; i++) {
      _data[i] += rhs[i];
    }
    return (*this);
  }

  index<DIMS>& operator -= (const index<DIMS>& rhs) {
    for(size_t i = 0; i < DIMS; i++) {
      _data[i] -= rhs[i];
    }
    return (*this);
  }

  index<DIMS> operator - (const index<DIMS>& rhs) {
    return (index<DIMS>(*this) -= rhs);
  }

  index<DIMS> operator + (const index<DIMS>& rhs) {
    return (index<DIMS>(*this) += rhs);
  }

  index_base<DIMS>& operator = (const index_base<DIMS>& rhs) {
    _data = rhs.data();
    return (*this);
  }
};

template<size_t DIMS>
class index;

template<>
class index<1> : public index_base<1>
{
public:
  index() : index_base<1>() { }

  index(const std::array<size_t, 1>& rhs) : index_base<1>(rhs) { }

  explicit index(size_t i) {
     _data[0] = i;
  }

  void print() const {
    MARE_ALOG("(%zu)", _data[0]);
  }
};

template<>
class index<2> : public index_base<2>
{
public:
  index() : index_base<2>() { }

  index(const std::array<size_t, 2>& rhs) : index_base<2>(rhs) { }

  index(size_t i, size_t j) {
     _data[0] = i;
     _data[1] = j;
  }

  void print() const {
    MARE_ALOG("(%zu, %zu)", _data[0], _data[1]);
  }
};

template<>
class index<3> : public index_base<3>
{
public:
  index() : index_base<3>() { }

  index(const std::array<size_t, 3>& rhs) : index_base<3>(rhs) { }

  index(size_t i, size_t j, size_t k) {
     _data[0] = i;
     _data[1] = j;
     _data[2] = k;
  }

  void print() const {
    MARE_ALOG("(%zu, %zu, %zu)", _data[0], _data[1], _data[2]);
  }
};

} // ns internal

} // ns mare
