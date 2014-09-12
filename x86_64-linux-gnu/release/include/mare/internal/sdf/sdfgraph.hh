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
/** @file sdfgraph.hh */
#pragma once

#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
#include <vector>

#include <mare/internal/sdf/channelimplementation.hh>
#include <mare/internal/sdf/sdfpartition.hh>

#define MARE_SDF_WITHIN_PARTITION_CHANNEL_BUFFER_LENGTH 1

namespace mare {
namespace internal {

void set_all_src_dst(
  sdf_node_common*                sdf_node,
  std::vector<direction> const&   vdir,
  std::vector<std::size_t> const& vsizes,
  std::vector<channel*> const&    vchannels);

  /// sdf_graph_status:
  /// captures a graph's current execution status
  ///
  /// Also, captures which pause request (if any) has currently
  /// paused the graph -- referred to when graph is resumed.
  ///
  /// Note that a cancelled graph will have both _is_cancelled
  ///  and _is_completed set to true.

class sdf_graph_status {
  /// only sdf_graph can set the fields
  friend class sdf_graph;

  bool _is_launched;
  bool _is_executing;
  bool _is_paused;
  bool _is_cancelled;
  bool _is_completed;

  /// non-zero iff _is_paused == true
  std::condition_variable* _p_paused_by_condvar;

public:
  sdf_graph_status() :
    _is_launched(false),
    _is_executing(false),
    _is_paused(false),
    _is_cancelled(false),
    _is_completed(false),
    _p_paused_by_condvar(0) {}

  inline bool get_is_launched() const
  {
    return _is_launched;
  }

  inline bool get_is_executing() const
  {
    return _is_executing;
  }

  inline bool get_is_paused() const
  {
    return _is_paused;
  }

  inline bool get_is_cancelled() const
  {
    return _is_cancelled;
  }

  inline bool get_is_completed() const
  {
    return _is_completed;
  }
};


  /// sdf_graph:
  /// functionality for capturing the nodes and channels of the graph,
  ///    graph analysis, partitioning decisions, launch and interruption.
  ///
  /// Key mechanisms:
  ///  - Graph representation is constructed by user API prior to launch:
  ///      + set of nodes and channels:
  ///           _s_nodes
  ///           _s_channels
  ///
  ///      + methods:
  ///           add_node()
  ///           add_channel()
  ///
  ///  - After launch, graph elaboration ==> essentially generate more
  ///      convenient representation of graph structure
  ///       + map from node to its input and output intra-partition channels:
  ///           _m_in_channels
  ///           _m_out_channels
  ///
  ///       + topological sort of the graph nodes for scheduling:
  ///           _v_top_sorted_nodes
  ///
  ///       + method:
  ///           elaborate_graph_structure()
  ///
  ///  - After elaboration, do partitioning -- analysis, create partitions
  ///       + division of work over partitions -- vector of partitions,
  ///         and map from node to partition the node is assigned to:
  ///           _v_partitions
  ///           _m_node_to_partition
  ///
  ///       + inter-partition channels need to be treated differently from
  ///         intra-partision channels, maintain as a set
  ///           _s_interpartition_channels
  ///
  ///       + methods:
  ///           initial_partitioning()               (selectively calls one of
  ///                                                 the following)
  ///           do_automatic_node_partitioning()
  ///           apply_user_node_partitioning()
  ///
  ///  - After partitioning, execute partitions in phases:
  ///       + initial phase: by default ask partitions to execute the number
  ///         of graph iterations requested by the user (possibly infinite)
  ///       + if interrupted, SDF launcher signals executing partitions to stop
  ///         ASAP.
  ///       + API methods:
  ///            common_launch_and_wait()  (the SDF launcher, also elaborates
  ///                                       and partitions the graph)
  ///            common_launch()           (uses common_launch_and_wait())
  ///            wait()
  ///
  ///       + methods:
  ///            setup_step_num_iterations_from_active_req()
  ///            process_satisfaction_of_active_req()
  ///
  ///       + as a subsequent phase, re-launch all partitions to complete work
  ///         to a "synchronization point" (see description of
  ///         partition_iteration in sdfiteration.hh and the interruption
  ///         semantics in sdf.hh), or to execute till the graph iteration
  ///         originally specified by user.
  ///
  ///  - Concurrent to any of the above steps, the application can submit pause
  ///    and cancel requests. The requests are enqueued and processed in order.
  ///       + data structure:
  ///           _interrupt_request_q    (queue of user's interrupt requests)
  ///           _interrupt_immediately  (global flag checked by partitions to
  ///                                    terminate their execution ASAP to
  ///                                    allow the first interrupt request
  ///                                    to be processed and applied ASAP)
  ///
  ///       + method:
  ///           enqueue_interrupt_request() (enqueues a user interrupt,
  ///                                        asserts _interrupt_immediately
  ///                                        only if _interrupt_request_q
  ///                                        is empty when request enqueued)
  ///
  ///           pop_interrupt_request()  (invoked by common_launch_and_wait()
  ///                                     to launch a new phase)
  ///       + API methods:
  ///           common_pause()
  ///           common_cancel()
  ///                  (both culminate in call to enqueue_interrupt_request())
  ///
  ///       + if the first pause/cancel request arrives after the graph is
  ///         already launched, a single global flag is asserted to force
  ///         all the executing partitions to stop ASAP. The current phase of
  ///         partition execution is considered complete once all partitions
  ///         are stopped. Once the phase completes, the SDF launcher possibly
  ///         launches another short phase of partition execution so that all
  ///         partitions stop in a synchronized manner required by the
  ///         semantics of the pause/cancel request (see sdf.hh).
  ///
  ///       + if _interrupt_request_q is not empty when a request is enqueued,
  ///         the currently executing phase is not interrupted. Instead,
  ///         whenever a phase completes, the SDF launcher (method
  ///         common_launch_and_wait()) simply pops the next interrupt request
  ///         from _interrupt_request_q and launches the next phase to execute
  ///         *only as far as necessary* to satisfy the next interrupt
  ///         request. Hence, interruption of already executing partitions by
  ///         setting _interrupt_immediately = true is only done if a request
  ///         is enqueued to an empty _interrupt_request_q.
  ///
  ///       + methods:
  ///           setup_step_num_iterations_from_active_req()
  ///                     (how far should next phase execute to satisfy the
  ///                      last popped interrupt request, if any)
  ///           process_satisfaction_of_active_req()
  ///                     (after a phase ends, updates graph status in
  ///                      accordance to the nature of the last interrupt
  ///                      request, if any)
  ///
  ///       + note that the interrupt requests are processed by their order
  ///         in _interrupt_request_q. Therefore, a cancel request earlier
  ///         may leave the rest of the queue to have no effect. Or, if
  ///         an earlier pause request is for iteration 9000 and a later
  ///         pause request for iteration 5000, the graph would pause first
  ///         at iteration 9000, then on being resumed it would pause
  ///         immediately to service the second pause request
  ///         (see interrupt semantics in sdf.hh).
  ///
  ///  - A mutex protects the graph control interfaces, but the mutex is not
  ///    held for the partitions to execute a phase.
  ///       + mutex:
  ///           _control_mutex
  ///
  ///       + The following activities require _control_mutex to be held:
  ///          -- Elaboration, partitioning and launch of the initial phase
  ///          -- Enqueue of any pause/cancel request from the application
  ///          -- Post-analysis after a phase completes to determine if an
  ///             additional short phase must be launched so that all
  ///             partitions reach a synchronized point
  ///                 (process_satisfaction_of_active_req())
  ///          -- If the graph is neither completed nor cancelled by the
  ///             last completed phase, process the next enqueued request
  ///             and, if needed, launch a new phase to satisfy the request
  ///          -- Access to graph status, either to query or change
  ///               sdf_graph_status _status
  ///
  ///  - A single global flag is used to signal all partitions to stop ASAP.
  ///       + data structure:
  ///           volatile bool _interrupt_immediately;
  ///       + Semantics
  ///          -- Set to false in the SDF launcher before the launch of each
  ///             phase
  ///          -- If a pause/cancel is enqueued to an empty request queue and a
  ///             phase is currently executing, the flag is set to true
  ///             (see method enqueue_interrupt_request())
  ///          -- The partition execution loops check the flag at various
  ///             points. If the flag tests true at any point, the loops are
  ///             terminated immediately and the resume point information is
  ///             saved within each partition.
  ///          -- Once all partitions have terminated, the MARE runtime returns
  ///             control to the SDF launcher (which may execute a short phase
  ///             to synchronize the resume points of the partitions).
  ///          -- Correctness without the use of locks or atomics:
  ///              ++ note that the false->true transition can happen at most
  ///                 once during the execution of the current phase
  ///              ++ note that we don't care when each partition detects the
  ///                 flag == true. Each checks independently, so long as each
  ///                 partition ultimately sees the flag == true or simply
  ///                 completes.
  ///              ++ note that the reverse transition flag = true->false
  ///                 does not occur until after all partitions have terminated
  ///                 and the current phase is completed.
  ///              ++ note that the request enqueuer is the only writer and
  ///                 the partition loops only read the flag.
  ///              ++ therefore, we rely on the monotonic update pattern to
  ///                 produce lock-free, atomic-free signalling.
  ///
  ///  - Resume after a pause
  ///       + Once a pause request has caused the partitions to stop execution
  ///         in a synchronized manner, the SDF launcher waits on a condition
  ///         variable to be signalled to indicate that the application code
  ///         has executed a resume user API call:
  ///            std::condition_variable  _resume_condvar
  ///
  ///       + The SDF launcher calls process_satisfaction_of_active_req() to
  ///         wait on its behalf after a pause.

class sdf_graph {

  /// Data structure representing an interrupt request within sdf_graph.
  /// See sdf.hh for interruption semantics for pause and cancel.
  struct sdf_interrupt_request {
    bool        _flag_iter_non_synced_immediate;

    bool        _flag_iter_non_synced_iteration;
    std::size_t _iter_non_synced_after_iteration;

    bool        _flag_iter_synced_immediate;

    bool        _flag_iter_synced_iteration;
    std::size_t _iter_synced_after_iteration;

    enum class requestor {kcancel, kpause};
    requestor   _requestor;

    /// defined iff _requestor==kpause
    std::mutex             * _p_pause_mutex;

    /// defined iff _requestor==kpause
    std::condition_variable* _p_pause_condvar;

    sdf_interrupt_request() :
      _flag_iter_non_synced_immediate(false),
      _flag_iter_non_synced_iteration(false),
      _iter_non_synced_after_iteration(0),
      _flag_iter_synced_immediate(false),
      _flag_iter_synced_iteration(false),
      _iter_synced_after_iteration(0),
      _requestor(requestor::kcancel),
      _p_pause_mutex(0),
      _p_pause_condvar(0) {}
  };

  ///////////////////////////////////
  ///////////// MEMBERS /////////////
  ///////////////////////////////////

  /// Serializes external control and query of the graph via the
  /// various API methods.
  std::mutex               _control_mutex;

  /// Interrupts execution of partition execution loops.
  ///
  /// Lock-free read by partition execution loops,
  /// but _control_mutex must be locked to write
  /// (set =true by method enqueue_interrupt_request(),
  ///  and =false by common_launch_and_wait())
  ///
  /// Monotonic update synchronization:
  /// once asserted, stays asserted until all possible
  /// readers (partitions) are confirmed to be interrupted.
  volatile bool            _interrupt_immediately;

  /// Set by the executing partitions if any of them
  /// interrupted their execution on detecting
  /// _interrupt_immediately == true.
  /// Read by SDF launcher at the end of each phase.
  /// If =true, SDF launcher checks if an additional
  /// phase of work needs to be executed to meet
  /// semantic requirements of the interrupt request.
  /// Access only with _control_mutex locked.
  ///
  /// Passed as a pointer to the partitions.
  /// Monotonic update as partitions never read this
  /// but some might set it =true.
  bool                     _was_interrupted;

  /// Holds the status of the graph
  /// (whether launched, executing, paused, cancelled,
  ///  and the graph iterations reached)
  ///
  /// Read and written from multiple methods invoked concurrently
  ///  (common_launch_and_wait(), common_pause(), get_status())
  /// Access only with _control_mutex locked.
  sdf_graph_status         _status;

  /// Condition variable signalled by resume() API method to allow
  /// method process_satisfaction_of_active_req() to unblock after
  /// the method waits to satisfy a pause request.
  /// See pause/resume semantics in sdf.hh:
  ///  - Mainly, the pause() API call blocks until the graph pauses
  ///    in response. Then it unblocks.
  ///  - However, the graph cannot resume execution until the user
  ///    executes a resume() API call. process_satisfaction_of_active_req()
  ///    is responsible for blocking the graph here.
  ///  - Error if a resume is called when graph is not paused.
  std::condition_variable  _resume_condvar;

  /// Indicates whether the phase currently being executed
  /// in the do-while loop in common_launch_and_wait() is
  /// servicing an interrupt request popped from _interrupt_request_q.
  ///
  /// Mostly used within common_launch_and_wait(), but also checked
  /// by enqueue_interrupt_request(), which needs to ascertain that
  /// both _interrupt_request_q is empty() and no popped request is
  /// currently being processed (as indicated by _has_active_req).
  /// Access only with _control_mutex locked.
  bool                     _has_active_req;

  /// Used only if launch is decoupled from wait (asynchronous launch).
  /// Using a task to perform the blocking launch via common_launch_and_wait()
  /// allows the user thread to not get blocked.
  task_ptr _launcher_task;

  /// flag, true iff _launcher_task defined.
  bool     _is_defined_launcher_task;

  std::chrono::time_point<std::chrono::system_clock>
                           _graph_creation_time;
  std::chrono::time_point<std::chrono::system_clock>
                           _graph_elaboration_start_time;
  std::chrono::time_point<std::chrono::system_clock>
                           _graph_partitioning_start_time;

  //// --- Initial Graph Description --- ////

  /// Set of nodes in the graph
  std::set<sdf_node_common*> _s_nodes;

  /// Set of channels in the graph
  std::set<channel*>         _s_channels;


  //// --- Graph Structure Elaboration --- ////

  /// Set of non-preloaded input channels for each node
  std::map< sdf_node_common*, std::set<channel*> > _m_in_channels;

  /// Set of non-preloaded output channels for each node
  std::map< sdf_node_common*, std::set<channel*> > _m_out_channels;

  /// Node schedule for whole graph, based on a topological sort
  /// of the DAG structure within the graph:
  ///   DAG = graph - preloaded-edges
  std::vector<sdf_node_common*> _v_top_sorted_nodes;


  //// --- Partitions for Graph Execution --- ////

  /// Set of partitions allocated
  std::vector<sdf_partition*> _v_partitions;

  /// Identify which partition executes a given node
  std::map<sdf_node_common*, sdf_partition*> _m_node_to_partition;

  /// Channels that cross partitions: src and dst of channel
  /// are nodes in different partitions
  std::set<channel*> _s_interpartition_channels;

  /// Default buffer size for channels within a partition
  std::size_t const _within_partition_channel_buffer_length;

  /// Default buffer size for channels that cross partitions
  std::size_t const _inter_partition_channel_buffer_length;


  //// --- Node Debug ID Generation --- ////

  /// Unique ID to assign the next node that is created
  /// within this graph. Incremented after each assignment.
  std::size_t _next_node_debug_id;


  //// --- Debug Interface --- ////

  /// Captures user-request for a specific number of partitions to be created,
  /// over-riding default mechanism to determine number of partitions
  std::size_t _user_num_partitions;

  /// Captures node-schedules manually created by the user for each partition.
  ///
  /// If specified by user, altogether disables automatic partitioning of graph
  ///
  /// Defined iff _user_num_partitions > 0.
  /// Assert: _user_partition_schedules.sizeof() == _user_num_partitions.
  ///
  /// Each element corresponds to a partition, and is a vector holding
  ///  the user-defined schedule.
  std::vector< std::vector<sdf_node_common*> > _user_partition_schedules;

  /// Map of assignment of a node to a partition integer ID.
  /// Allows reverse lookup of _user_partition_schedules
  std::map<sdf_node_common*, std::size_t> _map_node_to_user_defined_partition;


  //// --- Cost-based Automatic Partitioning --- ////

  /// Cost assigned by user to each node to drive automatic cost-based
  /// partitioning.
  ///
  /// Used if MARE_SDF_COST_BASED_PARTITIONING is defined.
  std::map<sdf_node_common*, double> _map_node_to_cost;


  //// --- Interruption Requests --- ////

  /// A queue of interrupt request received asynchronously
  /// from user application. The queue can hold requests
  /// even before the graph launches.
  std::list<sdf_interrupt_request> _interrupt_request_q;

  ///////////////////////////////////
  ///////////// METHODS /////////////
  ///////////////////////////////////

public:
  sdf_graph();
  ~sdf_graph();

  //// --- Graph Setup --- ////

  /// Adds a created node to the graph
  void add_node(sdf_node_common* n)
  {
    _s_nodes.insert(n);
  }

  /// Adds a channel to the graph
  void add_channel(channel* c)
  {
    _s_channels.insert(c);
  }


  //// --- Graph Manual Costs --- ////

  /// User can optionally specify relative costs for the nodes.
  /// These node costs are used during partitioning of the graph to
  /// attempt to create partitions with total costs as equal as possible.
  ///
  /// A cost = 1.00 is assumed for each node for which the user has not
  /// explicitly assigned a cost using assign_cost().
  void assign_cost(sdf_node_common* node_common, double execution_cost);


  //// --- Node Debug ID Generation --- ////

  /// Used to request a unique ID for a node being created, provided
  /// the created node will be added to this graph.
  /// The ID is unique only across nodes within the graph.
  std::size_t gen_next_node_debug_id()
  {
    return _next_node_debug_id++;
  }


  //// --- Debug Interface: Manual Partitioning --- ////

  /// Allows the user to override the default number of partitions
  /// determined best by MARE and SDF.
  void manual_set_num_partitions(std::size_t num_partitions)
  {
    MARE_API_ASSERT(_user_num_partitions == 0,
                    "user num_partitions is already set");
    _user_num_partitions = num_partitions;
  }

  /// Allows the user to explicitly assign a node to a specific
  /// partition identified by its integer ID. The partition IDs
  /// will run from 0 to num_partitions-1, for num_partitions set
  /// in a prior manual_set_num_partitions() call by the user.
  /// Error to invoke manual_assign_partition() without first
  /// invoking manual_set_num_partitions().
  void manual_assign_partition(
    sdf_node_common* node_common,
    std::size_t partition_index);

private:
  //// --- Graph Structure Elaboration --- ////

  /// Elaborates graph structure starting from _s_nodes and _s_channels
  void elaborate_graph_structure();


  //// --- Graph Partitioning --- ////

  /// invoked by initial_partitioning() for automatic partitioning
  void do_automatic_node_partitioning();

  /// invoked by initial_partitioning() for applying user's manual partitioning
  void apply_user_node_partitioning();

  /// Partitions the graph using the data structures generated by
  /// elaborate_graph_structure().
  /// If the user has manually specified partitioning, applies those
  /// specifications to partition the graph, otherwise partitions
  /// automatically.
  void initial_partitioning();


  //// --- Graph Execution and Interruption Support --- ////

  /// Return of false ==> an execution phase is not required to
  /// satisfy active_req (e.g., the interruption iteration
  /// desired in the request has already been passed).
  /// Otherwise, step_num_iterations is set to satisfy
  /// the active-request over the next execution phase.
  bool setup_step_num_iterations_from_active_req(
    bool const                   run_infinite_iterations,
    std::size_t const            num_iterations,
    sdf_interrupt_request const& req,
    partition_iteration const&   min_resume_piter,
    partition_iteration const&   max_resume_piter,
    std::size_t&                 step_num_iterations);

  /// Post-processes after completion of an execution
  /// phase, depending on nature of request active
  /// over the phase.
  /// cancel ==> no post-processing required.
  /// pause  ==> performs a conditional wait on _resume_condvar,
  ///            unless the graph has already completed.
  /// Returns whether request was a cancel.
  bool process_satisfaction_of_active_req(
    sdf_interrupt_request const& active_req);

  /// Refactored common functionality to support
  /// common_pause() and common_cancel() API when
  /// the user *has not* specified an iteration count
  /// for the interruption.
  void request_interrupt(
    sdf_interrupt_type               intr_type,
    sdf_interrupt_request::requestor requestor,
    std::mutex*                      p_pause_mutex,
    std::condition_variable*         p_pause_condvar);

  /// Refactored common functionality to support
  /// common_pause() and common_cancel() API when
  /// the user *has* specified an iteration count
  /// for the interruption.
  void request_interrupt(
    std::size_t                      desired_cancel_iteration,
    sdf_interrupt_type               intr_type,
    sdf_interrupt_request::requestor requestor,
    std::mutex*                      p_pause_mutex,
    std::condition_variable*         p_pause_condvar);

  /// Invoked by request_interrupt() variants to submit a fully
  /// constructed request into _interrupt_request_q.
  /// If _interrupt_request_q is empty and no popped request is
  /// currently being executed in common_launch_and_wait(), will
  /// also assert _interrupt_immediately = true to signal all
  /// partitions to stop ASAP. Otherwise, the partitions will run
  /// till completion as originally launched.
  ///
  /// If requests were enqueued or a request is currently being
  /// executed, it suffices to merely enqueue this new request,
  /// as the SDF launcher is already executing interruptions
  /// instead of running till completion for the original launch
  /// target.
  void enqueue_interrupt_request(sdf_interrupt_request const& req);

  /// Used by the execution phases loop in common_launch_and_wait()
  /// to pop the next interrupt request to execute.
  sdf_interrupt_request pop_interrupt_request();

public:
  //// --- Graph Execution and Interruption API --- ////

  /// The SDF launcher, consisting primarily of the execution phases
  /// do-while loop, each iteration of which sets the target of how far
  /// the partitions must execute the graph (each iteration of the loop
  /// is an execution phase).
  /// If no interrupt requests are encountered, a single phase executes
  /// the graph till completion.
  void common_launch_and_wait(
    bool const run_infinite_iterations,
    std::size_t const num_iterations);

  /// Wraps a launcher task around the blocking common_launch_and_wait()
  /// to allow the application thread launching the graph to not block.
  /// (Asynchronous launch)
  void common_launch(
    bool run_infinite_iterations,
    std::size_t num_iterations);

  /// Blocks until graph completion (including by cancellation).
  /// Allows the user API wait_for() calls to achieve blocking.
  void wait();

  /// Underlying functionality for user API pause() calls.
  bool common_pause(
    bool          is_immediate_pause,
    std::size_t   desired_pause_iteration,
    sdf_interrupt_type intr_type);

  /// Underlying functionality for user API resume().
  void resume();

  /// Underlying functionality for user API cancel() calls.
  void common_cancel(
    bool          is_immediate_cancel,
    std::size_t   desired_cancel_iteration,
    sdf_interrupt_type intr_type);

  /// Invoked by a partition when a node it last executed got blocked channels
  /// or detected that _interrupt_immediately was asserted.
  /// The channel states are setup so that the node can be re-executed later
  /// and only the channels that had not already popped/pushed will attempt to
  /// pop/push.
  void setup_channel_resumes(sdf_node_common* n);


  //// --- Graph Query API --- ////

  /// Gets current status of the graph.
  /// If the graph has launched but the partitions are
  /// currently not executing, iteration information
  /// is collected from across the partitions, and the
  /// min and max resume points across the partitions are
  /// reported in min_resume_piter and max_resume_piter.
  sdf_graph_status get_status(
    partition_iteration& min_resume_piter,
    partition_iteration& max_resume_piter);

private:
  MARE_DELETE_METHOD(sdf_graph(sdf_graph const&));
  MARE_DELETE_METHOD(sdf_graph operator=(sdf_graph const&));
  MARE_DELETE_METHOD(sdf_graph(sdf_graph&&));
  MARE_DELETE_METHOD(sdf_graph operator=(sdf_graph&&));
};


//////////

  /// Services all variants of mare::launch_and_wait() API calls
  /// without exposing class sdf_graph to user
void common_launch_and_wait(
  sdf_graph_ptr g,
  bool const run_infinite_iterations,
  std::size_t const num_iterations);


  /// Services all variants of asynchronous mare::launch() API calls
  /// without exposing class sdf_graph to user
void common_launch(
  sdf_graph_ptr g,
  bool run_infinite_iterations,
  std::size_t num_iterations);


  /// Services all variants of mare::pause() API calls
  /// without exposing class sdf_graph to user
bool common_pause(
  sdf_graph_ptr g,
  bool          is_immediate_pause,
  std::size_t   desired_pause_iteration,
  sdf_interrupt_type intr_type);


  /// Services all variants of mare::cancel() API calls
  /// without exposing class sdf_graph to user
void common_cancel(
  sdf_graph_ptr g,
  bool          is_immediate_cancel,
  std::size_t   desired_cancel_iteration,
  sdf_interrupt_type intr_type);

} //namespace internal
} //namespace mare

