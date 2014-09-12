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
/** @file sdfpr.hh */
#pragma once

#include <vector>

#include <mare/internal/sdf/sdfbasedefs.hh>

namespace mare {

// ---------------------------------------------------------------------------
// USER API
// ---------------------------------------------------------------------------
/** @addtogroup sdf_doc
@{ */

//////////////
// Programmatic Graph creation

/**
  Introspection data-structure used by the programmatic body of an SDF node.

  In addition to using <tt>body(T1& t1, T2& t2, ..., Tn& tn)</tt>, where
  the type signature is fixed at compile-time and a priori fixes the
  data-types, direction and number of channels connecting to that body's
  SDF node, MARE SDF allows a programmatic introspection body with the
  following type signature that can discover the nature of channels
  connected to it:<BR>
     <tt>body(node_channels& ncs)</tt>

  Inside the body, the user-code can use the <tt>ncs</tt> object to query the
  number of channels connected, and for each channel query its direction
  (in or out) and the element-size carried by it. While node_channels
  provides the user the flexibility to attach however many channels of
  whatever user-data-types to a node, type-safety is reduced as there is no
  longer compile-time type matching between the type signature of
  <tt>body</tt> and the types and number of channels attached to the node.

  read() and write() methods inside node_channels allow the user-code to
  access data popped from in-channels and provide data to push to out-channels.
  The user-code must pass program variables by reference, whose sizes must
  match the element-sizes of the corresponding channels. read() and write()
  provide some runtime safety by checking that the size of the passed program
  variable matches the element-size of the channel.

  Runtime error if a write is attempted on an in-channel
  or if a read is attempted on an out-channel.
*/
class node_channels {
public:
  node_channels() :
    _vdir(),
    _velemsize(),
    _vbuffer() { }

/**
    Number of channels connected to the node.
    Channels are accessed by their indices:
        0 to get_num_channels()-1

    @return
    Number of channels connected the node.
*/
  std::size_t get_num_channels() const;

/**
    Determines if the specified channel is an in-channel.

    @param channel_index Index to a connected channel.

    @return
    TRUE if in-channel to the node.
    FALSE otherwise.
*/
  inline bool is_in_channel(std::size_t channel_index) const;

/**
    Determines if the specified channel is an out-channel.

    @param channel_index Index to a connected channel.

    @return
    TRUE if out-channel to the node.
    FALSE otherwise.
*/
  inline bool is_out_channel(std::size_t channel_index) const;


/**
    Retrieves element-size carried by the specified channel.

    @param channel_index Index to a connected channel.

    @return
    Element-size carried by the specified channel.
*/
  std::size_t get_elemsize(std::size_t channel_index) const;

/**
    Copies a popped value from the specified channel into
    a program variable of data-type <tt>T</tt>.

    Multiple reads on a channel are allowed, each returning the
    same popped value (since the channel pop occurred prior to the
    invocation of the node's body).

    Runtime error if <tt>sizeof(T) != get_elemsize(channel_index)</tt>.<BR>
    Runtime error if <tt>!is_in_channel(channel_index)</tt>.

    @param t Reference to a program variable to which the popped
    value will be copied.

    @param channel_index Index to a connected channel.
*/
  template<typename T>
  void read(T& t, std::size_t channel_index);

/**
    Saves the value of a program variable of data-type <tt>T</tt>.
    The saved value will subsequently be pushed to the specified channel.

    Multiple writes on a channel are allowed, with only the
    final write providing the value pushed (channel push will
    occur after completion of the node's body).

    Runtime error if <tt>sizeof(T) != get_elemsize(channel_index)</tt>.<BR>
    Runtime error if <tt>!is_out_channel(channel_index)</tt>.

    @param t Reference to a program variable whose value will
    be saved for pushing to the channel.

    @param channel_index Index to a connected channel.
*/
  template<typename T>
  void write(T const& t, std::size_t channel_index);

private:
  friend class internal::node_channels_accessor;
  std::vector<internal::direction> _vdir;
  std::vector<std::size_t>         _velemsize;
  std::vector<char*>               _vbuffer;
};


/**
  A tuple holding a channel's binding to an in or out direction.

  Tuple is not templated on the user-data-type of the channel data.
  Suitable for programmatic construction of graph.

  @sa as_in_channel_tuple() and as_out_channel_tuple() produce
  bindings of type tuple_dir_channel.
*/
typedef std::tuple<internal::direction, channel*> tuple_dir_channel;

/**
  Binds a channel as in-channel for connecting to a node.

  Used for connecting the channel to a programmatically connected node.

  @param dc A data-channel templated on a user-data-type.

  @return
  A template-free binding of the channel as in-channel, suitable for
  passing around to generic parts of the application that are not
  specific to the user-data-type used for channel construction.
*/
template<typename T>
tuple_dir_channel as_in_channel_tuple(data_channel<T>& dc)
{
  return std::make_tuple(internal::direction::in,
                         dynamic_cast<channel*>(&dc));
}

/**
  Binds a channel as out-channel for connecting to a node.

  Used for connecting the channel to a programmatically connected node.

  @param dc A data-channel templated on a user-data-type.

  @return
  A template-free binding of the channel as out-channel, suitable for
  passing around to generic parts of the application that are not
  specific to the user-data-type used for channel construction.
*/
template<typename T>
tuple_dir_channel as_out_channel_tuple(data_channel<T>& dc)
{
  return std::make_tuple(internal::direction::out,
                         dynamic_cast<channel*>(&dc));
}


/**
  Programmatic connection of a node to channels, using a body
  with programmatic introspection.

  The signature of <tt>body</tt> does not carry template information
  on the user-data-types of the connected channels, the number of
  connected channels or their directions.

  @param g Handle to the SDF graph in which the node is to created.

  @param body An introspection function on node_channels.

  @param v_dir_channels Captures the channels connected to the node
  along with their directions. Expressed as a vector of channel-direction
  bindings.

  @return
  A handle to the created node.

  @sa node_channels on how <tt>body</tt> can introspect on the
  channels connected to it.

  @sa as_in_channel_tuple() for creation of an in-channel binding.

  @sa as_out_channel_tuple() for creation of an out-channel binding.
*/
inline
sdf_node_ptr create_sdf_node(
  sdf_graph_ptr g,
  void (* body)(node_channels&),
  std::vector<tuple_dir_channel>& v_dir_channels);

/**
  Programmatic connection of a node to channels, using a body
  with type-signature listing the user-data-types of connected
  channels.

  <tt>body</tt> is a callable (e.g., a function, object with operator(),
  a lambda with capture) whose callable type-signature lists the
  user-data-types of the connected channels. <tt>body</tt> is
  callable with a type signature like the following:<BR>
    <tt>body(T1& t1, T2& t2, ..., Tn& tn)</tt>

  where <tt>T1</tt> to <tt>Tn</tt> are the user-data-types of the
  channels connected to the node, though not visible to the compiler
  in the channel descriptions in <tt>v_dir_channels</tt>.

  The channels are expressed without user-data-type information. This
  form of create_sdf_node() is useful when the channels may have been
  passed through generic parts of the application code that was not
  specialized to the user-data-types used in channel creation, but
  node creation occurs within specialized code aware of the channel
  user-data-types.

  @param g Handle to the SDF graph in which the node is to created.

  @param body A callable with type-signature explicitly listing the
  user-data-types of the connected channels.

  @param v_dir_channels Captures the channels connected to the node
  along with their directions. Expressed as a vector of channel-direction
  bindings.

  @return
  A handle to the created node.

  @sa as_in_channel_tuple() for creation of an in-channel binding.

  @sa as_out_channel_tuple() for creation of an out-channel binding.
*/
template<typename Body>
sdf_node_ptr create_sdf_node(
  sdf_graph_ptr g,
  Body&& body,
  std::vector<tuple_dir_channel>& v_dir_channels);

/** @} */ /* end_addtogroup sdf_doc */

} //namespace mare
