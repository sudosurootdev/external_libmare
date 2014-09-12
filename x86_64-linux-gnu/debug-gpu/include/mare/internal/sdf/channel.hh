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

#include <tuple>
#include <vector>

#include <mare/exceptions.hh>
#include <mare/internal/debug.hh>
#include <mare/internal/macros.hh>

#include <mare/channel.hh>


namespace mare {
namespace internal {

  /// The channel class (see mare/channel.hh) is an abstract handle visible to
  /// the user. During execution, the SDF runtime needs to perform operations
  /// on the channel. The following operations are provided to interface with
  /// the channel's underlying implememtation. The pop/push-related functions
  /// described below are called by the pop_inputs/push_outputs functions
  /// defined in sdfhidden.hh.
  ///
  ///  - read()/write() an element from/to a raw char buffer holding the
  ///    channel data.
  ///     + the spin_on_blocked_channel flag dictates if the read/write
  ///       should spin until success (i.e., until there is data in the
  ///       channel to be read, or space available to write, respectively).
  ///     + return whether operation succeeded
  ///
  ///  - pop_buf() is a simple wrapper to attempt a non-spinning
  ///    read of an element from the channel to a local buffer.
  ///     + return whether operation succeeded.
  ///
  ///  - push_buf() is a simple wrapper for writing, corresponding to
  ///    pop_buf_value()
  ///
  ///  - pop_value() and push_value() provide pop/push of an element of
  ///    channel-specific data-type, providing type-safety. They wrap
  ///    read() and write(), respectively.
  ///     + return whether operation succeeded
  ///
  ///  - preload_allocate() is called by the SDF API preload_channel()
  ///    to allocate buffer space within the channel implementation to hold
  ///    the channel initialization elements provided by the user. Normally,
  ///    buffer space is allocated for a channel only at graph launch time.

bool read(channel * c, char* buffer, bool spin_on_blocked_channel);
    //returns true if channel read, false if interrupted and retry needed

bool write(channel * c, char const* buffer, bool spin_on_blocked_channel);
    //returns true if channel written, false if interrupted and retry needed

template<typename T>
bool pop_value(data_channel<T>& dc, T& t)
{
  channel* c = dynamic_cast<channel*>(&dc);
  return read(c, reinterpret_cast<char*>(&t), false);
    //returns true if read successfully, false if must be re-tried
}

inline
bool pop_buf(channel* c, void* buf)
{
  return read(c, reinterpret_cast<char*>(buf), false);
    //returns true if read successfully, false if must be re-tried
}


template<typename T>
bool push_value(data_channel<T>& dc, T const& t)
{
  channel* c = dynamic_cast<channel*>(&dc);
  return write(c, reinterpret_cast<char const*>(&t), false);
    //returns true if written successfully, false if must be re-tried
}

inline
bool push_buf(channel* c, void const* buf)
{
  return write(c, reinterpret_cast<char const*>(buf), false);
    //returns true if written successfully, false if must be re-tried
}

void preload_allocate(channel* c, std::size_t num_elems);

  /// map_data_channel_ref_to_channel_ptr:
  /// utility to convert a variadic list of data_channels (each of arbitrary
  /// user-defined-type) to a vector of untyped channel pointers
  /// (channel is the base type of data_channel<T>)

template<typename T, typename ...Ts>
void
map_data_channel_ref_to_channel_ptr_helper(
  std::vector<channel*>& vchannels,
  data_channel<T>& dc,
  data_channel<Ts>&... rest_dcs)
{
  vchannels.push_back( dynamic_cast<channel*>(&dc) );

  map_data_channel_ref_to_channel_ptr_helper(vchannels, rest_dcs...);
}

template<typename ...Ts>
std::enable_if<(sizeof...(Ts) == 0), void>
map_data_channel_ref_to_channel_ptr_helper(
  std::vector<channel*>& ,
  data_channel<Ts>&... )
{ }

template<typename ...Ts>
std::vector<channel*> map_data_channel_ref_to_channel_ptr(
  data_channel<Ts>&... dcs)
{
  std::vector<channel*> vchannels;

  map_data_channel_ref_to_channel_ptr_helper(vchannels, dcs...);

  MARE_INTERNAL_ASSERT(vchannels.size() == sizeof...(Ts),
                       "vchannels produced of incorrect length");

  return vchannels;
}

  /// map_data_channel_tuple_to_channel_ptr:
  /// utility to convert a tuple of data_channels (each of arbitrary
  /// user-defined-type) to a vector of untyped channel pointers
  /// (channel is the base type of data_channel<T>)

template<size_t remaining, typename ...Ts>
typename std::enable_if<(remaining == 0), void>::type
map_data_channel_tuple_to_channel_ptr_helper(
  std::vector<channel*>&,
  std::tuple< data_channel<Ts>*... >&)
{ }

template<size_t remaining, typename ...Ts>
typename std::enable_if<(remaining > 0), void>::type
map_data_channel_tuple_to_channel_ptr_helper(
  std::vector<channel*>& vchannels,
  std::tuple< data_channel<Ts>*... >& pdc_tuple)
{
  vchannels.push_back(
                dynamic_cast<channel*>(
                  std::get<sizeof...(Ts) - remaining>(pdc_tuple) ) );

  map_data_channel_tuple_to_channel_ptr_helper<remaining-1, Ts...>
                                              (vchannels, pdc_tuple);
}

template<typename ...Ts>
std::vector<channel*> map_data_channel_tuple_to_channel_ptr(
  std::tuple< data_channel<Ts>*... >& pdc_tuple)
{
  std::vector<channel*> vchannels;

  map_data_channel_tuple_to_channel_ptr_helper<sizeof...(Ts), Ts...>
                                              (vchannels, pdc_tuple);

  MARE_INTERNAL_ASSERT(vchannels.size() == sizeof...(Ts),
                       "vchannels produced of incorrect length");

  return vchannels;
}

  /// map_data_channel_ref_to_Tsize:
  /// utility to extract as a vector the element-size of each channel from a
  /// variadic list of data_channels (each of arbitrary user-defined-type)


template<typename ...Ts>
std::enable_if<(sizeof...(Ts) == 0), void>
map_data_channel_ref_to_Tsize_helper(
  std::vector<std::size_t>& ,
  data_channel<Ts>&... )
{ }

template<typename T, typename ...Ts>
void
map_data_channel_ref_to_Tsize_helper(
  std::vector<std::size_t>& vsizes,
  data_channel<T>& ,
  data_channel<Ts>&... rest_dcs)
{
  vsizes.push_back( sizeof(T) );

  map_data_channel_ref_to_Tsize_helper(vsizes, rest_dcs...);
}

template<typename ...Ts>
std::vector<std::size_t> map_data_channel_ref_to_Tsize(
  data_channel<Ts>&... dcs)
{
  std::vector<std::size_t> vsizes;

  map_data_channel_ref_to_Tsize_helper(vsizes, dcs...);

  MARE_INTERNAL_ASSERT(vsizes.size() == sizeof...(Ts),
                       "vsizes produced of incorrect length");

  return vsizes;
}

  /// map_var_tuple_to_Tsize:
  /// utility to extract as a vector the element-size of each element of
  /// a tuple

template<size_t remaining, typename ...Ts>
typename std::enable_if<(remaining == 0), void>::type
map_var_tuple_to_Tsize_helper(
  std::vector<std::size_t>& ,
  std::tuple<Ts...>& )
{ }

template<size_t remaining, typename ...Ts>
typename std::enable_if<(remaining > 0), void>::type
map_var_tuple_to_Tsize_helper(
  std::vector<std::size_t>& vsizes,
  std::tuple<Ts...>& var_tuple)
{
  vsizes.push_back( sizeof(std::get<sizeof...(Ts) - remaining>(var_tuple)) );

  map_var_tuple_to_Tsize_helper<remaining-1, Ts...>(vsizes, var_tuple);
}

template<typename ...Ts>
std::vector<std::size_t> map_var_tuple_to_Tsize(
  std::tuple<Ts...>& var_tuple)
{
  std::vector<std::size_t> vsizes;

  map_var_tuple_to_Tsize_helper<sizeof...(Ts), Ts...>(vsizes, var_tuple);

  MARE_INTERNAL_ASSERT(vsizes.size() == sizeof...(Ts),
                       "vsizes produced of incorrect length");

  return vsizes;
}


} //namespace internal

  /// See documentation in mare/channel.hh
template<typename T, typename Trange>
void preload_channel(data_channel<T>&dc, Trange const& tr)
{
  channel* c = dynamic_cast<channel*>(&dc);
  internal::preload_allocate(c, tr.size());

  for(std::size_t i=0; i<tr.size(); i++) {
    internal::push_value(dc, tr[i]);
  }
}

} //namespace mare
