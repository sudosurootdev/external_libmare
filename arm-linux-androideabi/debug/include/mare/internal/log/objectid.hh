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

#include <atomic>
#include <cstdint>

// GCC 4.6 moans about base class having a non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");

namespace mare {
namespace internal {
namespace log {

template <typename OBJECT_TYPE>
struct object_id_base {
  typedef std::uint32_t raw_id_type;
  typedef OBJECT_TYPE object_type;
  static constexpr raw_id_type INVALID_OBJECT_ID = 0;
  static constexpr raw_id_type FIRST_VALID_OBJECT_ID = INVALID_OBJECT_ID + 1;
};

// Always-zero object id
template <typename OBJECT_TYPE>
class null_object_id : public object_id_base<OBJECT_TYPE> {

  typedef object_id_base<OBJECT_TYPE> parent;

public:

  typedef typename parent::raw_id_type raw_id_type;

  raw_id_type get_raw_value() const { return parent::INVALID_OBJECT_ID; }

}; //null_object_id


template <typename OBJECT_TYPE>
class seq_object_id : public object_id_base<OBJECT_TYPE> {

  typedef object_id_base<OBJECT_TYPE> parent;

public:

  typedef typename parent::raw_id_type raw_id_type;

  seq_object_id()
    :_id(s_counter.fetch_add(1, std::memory_order_relaxed)) {}

  seq_object_id(seq_object_id const& other)
    :_id(other._id) {}

  seq_object_id(null_object_id<OBJECT_TYPE> const& other)
    :_id(other.get_raw_value()) {}

  raw_id_type get_raw_value() const { return _id; }

private:

  raw_id_type _id;
  static std::atomic<raw_id_type> s_counter;
}; //seq_object_id

template<typename OBJECT_TYPE>
std::atomic<typename object_id_base<OBJECT_TYPE>::raw_id_type>
  seq_object_id<OBJECT_TYPE>::
  s_counter(object_id_base<OBJECT_TYPE>::FIRST_VALID_OBJECT_ID);

} //mare::internal::log
} //mare::internal
} //mare

MARE_GCC_IGNORE_END("-Weffc++");
