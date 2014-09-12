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
/** @file channel.hh */
#pragma once

#include <mare/internal/macros.hh>

namespace mare {
namespace internal {

class channel_internal_record;
class channel_accessor;

} //namespace internal
} //namespace mare

namespace mare {

// ---------------------------------------------------------------------------
// USER API
// ---------------------------------------------------------------------------
/** @addtogroup sdf_doc
@{ */

/**
  Base class representing a channel connection between two SDF nodes.

  Cannot be instantiated directly by user. User code must instantiate the
  derived class <tt>data_channel<T></tt> to create a channel carrying elements
  of user-defined-type <tt>T</tt>.

  However, user code can pass pointers of the base type <tt>channel*</tt>
  and use those to connect SDF nodes instead of the templated
  <tt>data_channel<T></tt>. This is useful for the programmatic construction
  of an SDF graph, where the number and user-data-types of channels connected
  to a node may not be fixed at compile-time: specialized user code may create
  typed data_channels, while generic user code may only use <tt>channel*</tt>
  to connect the graph.

  @sa data_channel
*/
class channel {
  friend class internal::channel_accessor;
public:
  /**
    Returns <tt>sizeof(T)</tt> corresponding to the user-data-type <tt>T</tt>
    used to construct the channel <tt>data_channel<T></tt>.

    Provides some introspection or debugging capability to generic user-code
    when the application is divided into specialized user-code that creates
    <tt>data_channel<T></tt>, but only <tt>channel*</tt> is passed to the
    generic user code.

    @return
    <tt>sizeof(T)</tt> for <tt>T</tt> used in channel construction via
    <tt>data_channel<T></tt>
  */
  std::size_t get_elem_size() const;

protected:
  channel(std::size_t Tsize);
  virtual ~channel();

private:
  internal::channel_internal_record* _cir;
  MARE_DELETE_METHOD(channel(channel const&));
  MARE_DELETE_METHOD(channel(channel&&));
  MARE_DELETE_METHOD(channel& operator=(channel const&));
  MARE_DELETE_METHOD(channel& operator=(channel&&));
};

/**
  Construct a channel carrying elements of arbitrary user-data-type
  <tt>T</tt>.

  A channel must be connected as input to exactly one SDF node and as output to
  exactly one SDF node (possibly the same, if "delays" are added on the
  channel), before the launch of a graph containing the nodes. Both nodes must
  belong to the same SDF graph.

  @sa channel the base class.
  @sa preload_channel() for adding delays on a channel.
*/
template<typename T>
class data_channel : public channel {
public:
  data_channel() :
    channel(sizeof(T)) { }

  MARE_DELETE_METHOD(data_channel(data_channel<T> const&));
  MARE_DELETE_METHOD(data_channel(data_channel<T>&&));
  MARE_DELETE_METHOD(data_channel& operator=(data_channel<T> const&));
  MARE_DELETE_METHOD(data_channel& operator=(data_channel<T>&&));
};

/**
  Adds delay initializer elements to a channel.

  Edges in a synchronous dataflow graph can have delays associated with them.
  Any cycle in the graph must have at least one delay on at least one of the
  edges constituting the cycle in order to be a valid synchronous dataflow
  graph. In general, additional delays in a cycle or delays on purely
  feed-forward paths alter the semantics of the graph.

  With MARE SDF, edges are implemented as channels. A delay of 'N'
  is associated with a channel by adding N initializer elements to the
  channel using preload_channel().

  @param dc
  A <tt>data_channel<T></tt> of user-defined-type <tt>T</tt>.

  @param tr
  A container of N elements of data-type <tt>T</tt>, where N is the
  desired number of delays to add to the channel. N must be > 0.
  The container can be any data-structure supporting range-based
  iteration.
*/
template<typename T, typename Trange>
void preload_channel(data_channel<T>& dc, Trange const& tr);

/** @} */ /* end_addtogroup sdf_doc */

} //namespace mare


namespace mare {
namespace internal {

  /// Direction is bound with a channel to designate the channel
  /// as either an input or output to an SDF node.
  ///
  /// @sa node_channels The programmatic API allows the body of an SDF node
  /// to introspect on the direction of the channels connected to it.
enum class direction {
  in,
  out};

} //namespace internal
} //namespace mare

