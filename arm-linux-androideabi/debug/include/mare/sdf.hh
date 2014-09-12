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
/** @file sdf.hh */
#pragma once

#include <mare/channel.hh>
#include <mare/internal/sdf/sdfapiimplementation.hh>
#include <mare/sdfpr.hh>

namespace mare {

// ---------------------------------------------------------------------------
// USER API
// ---------------------------------------------------------------------------
/** @addtogroup sdf_doc
@{ */

//////////////
// Graph creation

/**
  Creates an empty graph.

  Nodes must subsequently be created with
  create_sdf_node() calls prior to launching graph.

  @return
  Handle to the created graph.
*/
sdf_graph_ptr create_sdf_graph();

/**
  Destroys a graph.

  Invoke on graph g that has either not been launched,
  has completed execution, or has been cancelled.
  Undefined behavior if an executing graph is destroyed.

  @param g Handle to the graph to be destroyed.
*/
void destroy_sdf_graph(sdf_graph_ptr& g);


//////////////
// Node creation

/**
  Indicates that a list of data-channels will be inputs.

  @param dcs A variadic list of data-channels
  <tt>data_channel<T1></tt> to <tt>data_channels<Tn></tt> of
  user-defined-types <tt>T1</tt> to <tt>Tn</tt>.

  @return
  An object binding the data-channels to the input direction.
  Pass to create_sdf_node() to indicate that the created node
  will treat these channels as input.

  Due to some limitations in Visual Studio 2013 support for C++11,
  only upto 4 channels are supported in each with_inputs() call when
  compiling with VS 2013 (no limitation when compiling with gcc or clang).
*/
template<typename ...Ts>
io_channels<Ts...>
with_inputs(data_channel<Ts>&... dcs);

/**
  Indicates that a list of data-channels will be outputs.

  @param dcs A variadic list of data-channels
  <tt>data_channel<T1></tt> to <tt>data_channels<Tn></tt> of
  user-defined-types <tt>T1</tt> to <tt>Tn</tt>.

  @return
  An object binding the data-channels to the output direction.
  Pass to create_sdf_node() to indicate that the created node
  will treat these channels as output.

  Due to some limitations in Visual Studio 2013 support for C++11,
  only upto 4 channels are supported in each with_outputs() call when
  compiling with VS 2013 (no limitation when compiling with gcc or clang).
*/
template<typename ...Ts>
io_channels<Ts...>
with_outputs(data_channel<Ts>&... dcs);

/**
  Creates a new node within the specified SDF graph.

  The node needs a user-defined <tt>body</tt> and
  a sequence of input/output data-channels to connect to.

  The <tt>body</tt> is a function or a callable object
  supporting invocation as follows:
  @par
  <tt>body(T1& t1, T2& t2, T3& t3, ..., Tn& tn)</tt>

  where <tt>T1</tt> to <tt>Tn</tt> are the user-data-types of
  <tt>data_channel<T1></tt> to <tt>data_channel<T2></tt>
  connected to the node in the same order.

  Data-channels are connected as inputs or outputs to the node when passed
  as parameters wrapped in <tt>with_inputs()</tt> or <tt>with_outputs()</tt>,
  respectively, to the create_sdf_node() call.

  During the execution of the graph, the SDF runtime will invoke <tt>body</tt>
  with an argument list of references to elements of types <tt>T1</tt> to
  <tt>Tn</tt>, corresponding to a single element from each of the connected
  channels. All input channels would have been popped by the SDF runtime so
  <tt>body</tt> can read a value for each of those channels. <tt>body</tt>
  must subsequently write a value to the elements corresponding to the output
  channels, which the SDF runtime will subsequently push on those channels.

  @param g Handle to the graph in which this node is to be created.

  @param body A function or callable object that accepts references to
  an element of each of the connected channels. <tt>body</tt> can also be a
  programmatic introspection function.

  @param io_channels_groups
  A sequence of channels, in groups of <tt>io_channels</tt> created
  with_inputs() and/or with_outputs(). The ordering of the channels must
  correspond to the types of the parameters accepted by <tt>body</tt>.

  @return
  A handle to the created node.

  @sa node_channels for programmatic introspection body

  Due to some limitations in Visual Studio 2013 support for C++11,
  only upto 5 <tt>io_channels_groups</tt> are allowed when
  compiling with VS 2013 (no limitation when compiling with gcc or clang).
*/
template<typename Body, typename ...IOC_GROUPS>
typename std::enable_if<internal::is_MDCAW<IOC_GROUPS...>::value,
                        sdf_node_ptr>::type
create_sdf_node(
  sdf_graph_ptr g,
  Body&& body,
  IOC_GROUPS const&... io_channels_groups);

/**
  Assigns an execution cost to the node (cost defaults to 1.0, if unspecified).

  The current execution model of SDF needs node costs as estimates of the
  relative execution times of nodes, in order to perform load balancing.
  The next SDF release will perform load balancing without the use of
  assign_cost().

  @param n Handle to an SDF node.

  @param execution_cost The execution cost to assign to the node.
*/
void assign_cost(sdf_node_ptr n, double execution_cost);



//////////////
// Utilities

/**
  Retrieve the graph that contains the specified node.

  @param node Handle to an SDF node.

  @return
  Handle to the graph that contains the node.
*/
sdf_graph_ptr get_graph_ptr(sdf_node_ptr node);

/**
  Retrieves the unique integer ID automatically assigned to the node
  within its graph.

  At node creation (via create_sdf_node()), an integer ID is
  automatically generated and assigned to the node. The value
  of the ID starts from 0 for the first node created in the graph
  and increments for each subsequent node created in the same graph.
  The ID is provided only as a convenience for the user-code:
  to aid debugging, identify creation order and identify uniquely.
  The ID is fixed once the node is created.

  @param node Handle to an SDF node.

  @return
  The integer ID assigned to the node.
*/
std::size_t get_debug_id(sdf_node_ptr node);



//////////////
// Graph execution control

/**
  Launches execution of an SDF graph for an unbounded number of graph
  iterations, and blocks until the graph is cancelled.

  Runtime error if graph g was previously launched.
  Runtime error if graph g is structurally incomplete, i.e., there are
  channels unconnected on one end.
  Runtime error if graph g is not schedulable due to 0-delay cycles:
    a cycle must have at least one channel with at least one preloaded value.

  @param g Handle to the SDF graph to launch.

  @sa preload_channel()
*/
void launch_and_wait(sdf_graph_ptr g);

/**
  Launches execution of an SDF graph for a specified number of graph
  iterations, and blocks until the graph completes or is cancelled.

  All nodes run for exactly num_iterations, unless cancelled.

  @param g Handle to the SDF graph to launch.

  @param num_iterations Number of graph iterations to run g.

  @sa launch_and_wait(sdf_graph_ptr g)
  for the possible runtime errors on g.
*/
void launch_and_wait(sdf_graph_ptr g, std::size_t num_iterations);


/**
  Asynchronous launch of an SDF graph for an unbounded number of graph
  iterations. Does not block for the graph to launch or complete.

  @param g Handle to the SDF graph to launch.

  @sa launch_and_wait(sdf_graph_ptr g)
  for the possible runtime errors on g.
  @sa wait_for() to block on an asynchronously launched graph.
*/
void launch(sdf_graph_ptr g);

/**
  Asynchronous launch of an SDF graph for a specified number of graph
  iterations. Does not block for the graph to launch or complete.

  All nodes run for exactly num_iterations, unless cancelled.

  @param g Handle to the SDF graph to launch.

  @param num_iterations Number of graph iterations to run g.

  @sa launch_and_wait(sdf_graph_ptr g)
  for the possible runtime errors on g.
  @sa wait_for() to block on an asynchronously launched graph.
*/
void launch(sdf_graph_ptr g, std::size_t num_iterations);
  //Non-blocking launch variants

/**
  Blocks on an asynchronously launched graph to complete execution
  or be cancelled.

  Multiple wait_for() calls are allowed to block on same graph.
  A wait_for() invoked after the graph has already completed or
  been cancelled will return immediately.

  @param g Handle to the SDF graph to block on.
*/
void wait_for(sdf_graph_ptr g);
  //Blocks on a non-blocking launch.

/**
  Types of graph interruption. Used both for qualifying interruption requests
  and for characterizing the outcome of an interruption.

  - <tt>undef</tt>:
    - Interruption type is not set
    .

  - <tt>iter_non_synced</tt>:
    - At interruption, all nodes in the graph need not have executed the same
      number of graph iterations.
    .

  - <tt>iter_synced</tt>:
    - At interruption, all nodes in the graph must have completed exactly the
      same number of graph iterations.
    .
  .

  Note that <tt>iter_synced</tt> is a special case of <tt>iter_non_synced</tt>.
  It is possible for a pause() or cancel() to request interruption with
  <tt>iter_non_synced</tt> semantics, and the graph to interrupt with
  <tt>iter_synced</tt> semantics.

  @sa pause() and cancel() make use of sdf_interrupt_type to specify
  the interruption semantics when requesting an interruption of the
  graph %execution.

  @sa sdf_graph_query_info where a sdf_interrupt_type field provides
  status information on the graph %execution state after an interruption or
  completion of the graph.
*/
enum class sdf_interrupt_type {
  undef,
  iter_non_synced,
  iter_synced};

/**
  Cancels the execution of a graph ASAP.

  A graph can be cancelled
  - before it launches (on launch, graph will terminate without execution)
  - while it is executing
  - or, after it has already finished execution (no effect)

  cancel() returns immediately, while any wait_for() will block until
     the cancel takes effect.

  @param g Handle to SDF graph to cancel.

  @param intr_type Interruption type requested.

  - <tt>intr_type == iter_non_synced</tt>:
    - Non-iteration-synchronized cancel, no guarantee that all nodes will stop
      on the same graph iteration. But executing nodes are not interrupted.
    .

  - <tt>intr_type == iter_synced</tt>:
    - Iteration-synchronized cancel, all graph nodes complete the exact same
      number of graph iterations.
    .
  .

  @sa sdf_interrupt_type

  Okay to cancel either from program external to graph g or from
  within a node of g.

  @par Interruption Request Queue
  Each graph has its own interruption-request queue. cancel() enqueues an
  interruption request for the SDF runtime to process. Multiple incoming
  requests get enqueued and are processed in order by the graph's runtime.
  Similarly, pause() also enqueues an interruption requests into
  the same interruotion queue. When a cancel request earlier in the queue
  takes effect, it renders the remaining requests to have no effect
  (all pauses will unblock, resumes will have no effect).

  @sa wait_for(), pause(), resume()
*/
void cancel(
  sdf_graph_ptr g,
  sdf_interrupt_type intr_type = sdf_interrupt_type::iter_non_synced);

/**
  Cancels the execution of a graph after all nodes have
  reached or exceeded a specified number of graph iterations.

  Will *attempt* to cancel exactly when all nodes have completed exactly
  <tt>desired_cancel_iteration</tt> iteration. If the interruption queue
  already has a prior request that is not processed as yet, then it is
  *guaranteed* that the cancel will happen precisely when all nodes complete
  <tt>desired_cancel_iteration</tt> (i.e., <tt>iter_synced</tt> semantics),
  regardless of the <tt>intr_type</tt> in the cancel request.

  @param g Handle to the SDF graph to cancel.

  @param desired_cancel_iteration Graph iteration that all nodes must
  complete before cancel.

  @param intr_type Interruption type requested.

  - <tt>intr_type == iter_non_synced</tt>:
    - some nodes may execute additional iterations, though every node will
      execute till <tt>desired_cancel_iteration</tt> iteration.
    .
  - <tt>intr_type == iter_synced</tt>:
    - all nodes will complete the exact same number of iterations,
      possibly past the <tt>desired_cancel_iteration</tt> iteration.
    .
  .

  @sa sdf_graph_query()
  determines on which graph iteration the cancel took effect and with what
  interrution semantics.
*/
void cancel(
  sdf_graph_ptr g,
  std::size_t   desired_cancel_iteration,
  sdf_interrupt_type intr_type = sdf_interrupt_type::iter_non_synced);

/**
  Pauses the execution of a graph ASAP.

  A graph can be paused
  - before it launches (on launch, graph will pause before executing)
  - while it is executing
  - or, after it has already finished execution (no effect)

  pause() blocks until the graph has either paused execution in response to
  this particular pause request (not any other pause request), or has
  terminated for other reasons (such as a preceding cancel request in the
  graph's interruption queue).

  resume() invoked after a pause() causes the graph to resume execution
  precisely from where it paused.

  @param g Handle to SDF graph to pause.

  @param intr_type Interruption type requested.

  - <tt>intr_type == iter_non_synced</tt>:
    - Non-iteration-synchronized pause, no guarantee that all nodes will pause
      on the same graph iteration. But executing nodes are not interrupted.
    .

  - <tt>intr_type == iter_synced</tt>:
    - Iteration-synchronized pause, all graph nodes complete the exact same
      number of graph iterations.
    .
  .

  @return
  TRUE if this pause took effect<BR>
  FALSE if graph got terminated or had already completed.

  @sa sdf_interrupt_type

  @sa cancel(sdf_graph_ptr g, sdf_interrupt_type intr_type)
  for details on the interruption request queue.

  Okay to pause either from program external to graph g or from
  within a node of g.

  @sa resume()
*/
bool pause(
  sdf_graph_ptr g,
  sdf_interrupt_type intr_type = sdf_interrupt_type::iter_non_synced);

/**
  Pauses the execution of a graph after all nodes have
  reached or exceeded a specified number of graph iterations.

  @param g Handle to the SDF graph to pause.

  @param desired_pause_iteration Graph iteration that all nodes must
  complete before cancel.

  @param intr_type Interruption type requested.

  @return
  TRUE if this pause took effect<BR>
  FALSE if graph got terminated or had already completed.

  @sa pause(sdf_graph_ptr g, sdf_interrupt_type intr_type)
  for details on pause blocking semantics, interruption semantics and
  resume semantics.
*/
bool pause(
  sdf_graph_ptr g,
  std::size_t   desired_pause_iteration,
  sdf_interrupt_type intr_type = sdf_interrupt_type::iter_non_synced);

/**
  Resumes execution after the graph has been paused.

  Non-blocking.
  Runtime error if the graph was not paused when resume() is called.
  No effect if the graph is already completed or cancelled.
  Can be called from any thread or task, not necessarily from where pause
  was executed.

  @param g Handle to the SDF graph to pause.

  @sa pause(sdf_graph_ptr g, sdf_interrupt_type intr_type);
  @sa
  pause(
    sdf_graph_ptr g,
    std::size_t   desired_pause_iteration,
    sdf_interrupt_type intr_type)
*/
void resume(sdf_graph_ptr g);


/**
  Query information about the state of an SDF graph.

  Captures a sample of an SDF graph's execution state when
  sdf_graph_query() is used to query a graph's execution state.

  @sa sdf_graph_query()
  @sa to_string()
*/
class sdf_graph_query_info {
public:
  /**
    Indicates if graph has been launched and started execution.

    @return
    TRUE if the graph has already been launched and has started execution.<BR>
    FALSE otherwise.
  */
  bool was_launched()  const { return _was_launched; }

  /**
    Indicates if the graph has completed execution or been cancelled.

    @return
    TRUE if the graph has completed execution or been cancelled.<BR>
    FALSE otherwise.
  */
  bool has_completed() const { return _has_completed; }

  /**
    Indicates if the graph was launched and has been paused.

    @return
    TRUE if the graph was launched and has been paused.<BR>
    FALSE otherwise.
  */
  bool is_paused()     const { return _is_paused; }

  /**
    Provides the minimum graph iteration started by all graph nodes.

    - Defined iff <tt>has_completed() == true</tt> or
      <tt>is_paused() == true</tt>.
    - On <tt>iter_non_synced</tt> exit or pause, current_min_iteration()
      and current_max_iteration() together bound the range of iterations
      completed by all graph nodes.
    - On <tt>iter_synced</tt> exit or pause, both are equal. In contrast,
      both being equal does not imply <tt>iter_synced</tt>, since some nodes
      may have been interrupted in the middle of executing a node iteration.
    .

    @return
    The minimum graph iteration that every node in the graph has started
    or exceeded.

    @sa current_max_iteration()
  */
  std::size_t current_min_iteration() const
  {
    MARE_API_ASSERT(has_completed() || is_paused(),
        "Defined iff has_completed() == true or is_paused() == true");
    return _current_min_iteration;
  }

  /**
    Provides the maximum graph iteration completed by any graph node.

    - Defined iff <tt>has_completed() == true</tt> or
      <tt>is_paused() == true</tt>.
    - On <tt>iter_non_synced</tt> exit or pause, current_min_iteration()
      and current_max_iteration() together bound the range of iterations
      completed by all graph nodes.
    - On <tt>iter_synced</tt> exit or pause, both are equal. In contrast,
      both being equal does not imply <tt>iter_synced</tt>, since some nodes
      may have been interrupted in the middle of executing a node iteration.
    .

    @return
    The maximum graph iteration that a node anywhere in the graph
    has completed.

    @sa current_min_iteration()
  */
  std::size_t current_max_iteration() const
  {
    MARE_API_ASSERT(has_completed() || is_paused(),
        "Defined iff has_completed() == true or is_paused() == true");
    return _current_max_iteration;
  }

  /**
    Interruption type observed after the last cancel or pause request
    (if any) to take effect.

    @return
    The interruption type in effect when the query was made.<BR>
    <tt>undef</tt>, if the graph was not paused, cancelled or completed
      when the query was made.

    @sa sdf_interrupt_type
  */
  sdf_interrupt_type intr_type() const { return _intr_type; }

private:
  friend sdf_graph_query_info sdf_graph_query(sdf_graph_ptr g);
  friend std::string to_string(sdf_graph_query_info const& info);

  bool _was_launched;
  bool _has_completed;
  bool _is_paused;

  std::size_t _current_min_iteration;
  std::size_t _current_max_iteration;

  sdf_interrupt_type _intr_type;
};

/**
  Queries the state of an SDF graph.

  @param g Handle to graph to query.

  @return
  Query information that samples the current execution state of the graph.

  @sa sdf_graph_query_info for a description of the graph query information.
*/
sdf_graph_query_info sdf_graph_query(sdf_graph_ptr g);

/**
  Converts the graph query information to a string suitable for printing.

  @param info The graph query information.

  @return Query information formatted into a string suitable for printing.
*/
std::string to_string(sdf_graph_query_info const& info);

/**
  Insertion operator to output formatted graph query information.

  @param os   The output stream
  @param info The graph query information.
*/
template<typename OStream>
OStream& operator<<(OStream& os, sdf_graph_query_info const& info);

/** @} */ /* end_addtogroup sdf_doc */

} //namespace mare

