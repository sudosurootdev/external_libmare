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
/** @file sdfbasedefs.hh */
#pragma once

#include <tuple>

#include <mare/channel.hh>

namespace mare {
namespace internal {

class sdf_graph;
class sdf_node;

class node_channels_accessor;

} //namespace internal
} //namespace mare

namespace mare {

/**
  Handle to an SDF graph
*/
typedef internal::sdf_graph* sdf_graph_ptr;

/**
  Handle to an SDF node
*/
typedef internal::sdf_node*  sdf_node_ptr;

} //namespace mare

namespace mare {
namespace internal {

  /// data_channel_access_wrapper captures a channel-direction binding

template<typename T>
class data_channel_access_wrapper {
public:
  data_channel<T>* _pdc;
  direction        _dir;

  data_channel_access_wrapper(data_channel<T>& dc, direction dir) :
    _pdc(&dc),
    _dir(dir) { }
};

  /// multiple_data_channel_access_wrapper captures multiple channel-direction
  /// bindings produced by a with_inputs() or with_outputs() API call.

  /// A multiple_data_channel_access_wrapper object is flattened
  /// into a sequence of data_channel_access_wrapper objects.
  /// A sequence of multiple_data_channel_access_wrapper objects
  /// is flattened into a sequence of data_channel_access_wrapper objects
  /// before being passed to sub_create_sdf_node() calls which only accept
  /// data_channel_access_wrapper objects as paramaters.
  ///  (see flatten_multiple_mdcaws() in sdfapiimplementation.hh)

template<typename ...Ts>
class multiple_data_channel_access_wrapper {
public:
  typedef std::tuple<Ts...>          collection_sig_type;

  std::tuple< data_channel<Ts>*... > _tpdcs;
  direction                          _dir;

  multiple_data_channel_access_wrapper(data_channel<Ts>&... dcs,
                                       direction dir) :
    _tpdcs(&dcs...),
    _dir(dir) { }
};

} //namespace internal
} //namespace mare

namespace mare {

  /// See io_channels documentation in sdf.hh
template <typename ...Ts>
using io_channels = internal::multiple_data_channel_access_wrapper<Ts...>;

template<typename ...Ts>
io_channels<Ts...>
with_inputs(data_channel<Ts>&... dcs)
{
  return io_channels<Ts...>(dcs..., internal::direction::in);
}

template<typename ...Ts>
io_channels<Ts...>
with_outputs(data_channel<Ts>&... dcs)
{
  return io_channels<Ts...>(dcs..., internal::direction::out);
}

} //namespace mare
