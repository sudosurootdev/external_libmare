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
/** @file sdfpartition.hh */
#pragma once

#include <mare/internal/sdf/sdfiteration.hh>
#include <mare/sdf.hh>

namespace mare {
namespace internal {

  /// sdf_partition:
  /// executes a partition of SDF graph nodes.
  ///
  /// Main features:
  ///  1. method run_partition()
  ///   a. launch point of a partition
  ///   b. accepts iteration bounds:
  ///       - run graph till specified graph iteration, or for infinity
  ///   c. accepts p_interrupt_immediately parameter from the graph's
  ///      _interrupt_immediately parameter. Allows the graph to globally
  ///      request interruption of all executing partitions.
  ///      See sdfgraph.hh for details.
  ///
  ///  2. method run_resume_list()
  ///   a. the standard model is to run each graph iteration for all the nodes
  ///      in the partition before proceeding to the next graph iteration.
  ///   b. however, if a node is found blocked on a channel in graph
  ///      iteration i, run_resume_list() will create and manage additional
  ///      context so that graph iteration (i+1) can be run for nodes prior to
  ///      the blocked node, and so on if a blocked node is also found on
  ///      iteration (i+1)
  ///   c. the macro EXECUTE_AROUND_BLOCKED_CHANNELS must be defined for
  ///      the partition to attempt re-executing the schedule for iteration i+1
  ///      (otherwise, the standard model is used -- a blocked node blocks the
  ///      entire partition)
  ///   d. The goal here is to allow *pipelined execution* within a partition:
  ///      - if a node later in the schedule blocks, the nodes earlier in the
  ///        schedule can continue execution so long as the output channels
  ///        have buffer space available to store the additional results.
  ///
  ///  3. _resume_list: list of "resume contexts"
  ///   a. _resume_list is initialized with a single context element.
  ///      The single context element is initialized to tell the partition
  ///      to start execution at the first iteration of the graph.
  ///   b. run_resume_list() executes contexts in their order of appearance in
  ///      _resume_list. A context may:
  ///       - *complete*, and get erased from _resume_list (if it is not the
  ///         last context in _resume_list), or
  ///       - get *blocked*, either by a blocked node or the
  ///         "pipelining restriction" (described further below).
  ///   c. a new context is appended to the end of _resume_list if the last
  ///      context in _resume_list blocks:
  ///       - if the last context encounters a blocked node on graph
  ///         iteration i, a new context for iteration i+1 is appended
  ///         to _resume_list (if EXECUTE_AROUND_BLOCKED_CHANNELS defined).
  ///   d. the new context is initialized to partition_iteration(i+1, 0),
  ///      i.e., start executing the first node in the node-schedule for
  ///      graph iteration i+1. See sdfiteration.hh for partition_iteration.
  ///   e. if the newly appended context blocks "right away" (i.e., at the very
  ///      first node of the schedule), the partition yields the CPU to mare.
  ///   f. run_resume_list() attempts to execute a context earlier in the list
  ///      as far as possible in the node schedule, before attempting the next
  ///      context. If the current context is not blocked at any node and is
  ///      executed to completion, it is erased from _resume_list, unless it
  ///      is the only context remaining.
  ///   g. a context for iteration i can execute till just before node n in the
  ///      node schedule if the *previous* context in _resume_list is at n,
  ///      i.e., the current context *blocks* at partition_iteration(i, n-1),
  ///      if the previous context is at partition_iteration(i-1, n)
  ///      (pipeline restriction: node n must complete iteration i-1 before
  ///      attempting i).
  ///   h. a context c that is not the last context in _resume_list, can
  ///      *complete* its iteration i when it reaches the end of the node
  ///      schedule without getting blocked. At this point it is erased
  ///      from _resume_list. Note that the next context has already started
  ///      execution of iteration i+1 from the beginning of the node schedule,
  ///      so there is nothing left for the current context to execute beyond
  ///      the end of the node schedule for iteration i.
  ///   i. if the last context in _resume_list is found blocked at the first
  ///      node itself (so it is not possible to append a new context), a
  ///      mare::yield() is performed so the partition gives up the cpu for a
  ///      while until some inter-partition channels have a chance to become
  ///      unblocked.
  ///   j. whenever mare resumes execution of the partition after a yield, the
  ///      _resume_list is processed in-order as described above.
  ///
  ///      To summarize:
  ///       - a context for iteration i+1 gets appended to _resume_list when
  ///         the last context (executing iteration i) blocks.
  ///       - contexts complete and get erased in-order, until there is
  ///         only one context remaining.
  ///       - a context later in _resume_list cannot complete until all the
  ///         earlier contexts have completed (due to pipelining restriction).
  ///       - when there is only one context in _resume_list, the end of the
  ///         node schedule for iteration i causes the context to be updated
  ///         to the start of iteration i+1.
  ///
  ///  4. method run_partial_schedule()
  ///   a. determines the range of nodes in the node-schedule to be
  ///      executed within a context provided by run_resume_list()
  ///
  ///  5. method run_iter_schedule()
  ///   a. invoked by run_partial_schedule() to execute a single graph
  ///      iteration over an appropriate range of nodes, as dictated by
  ///      the context

class sdf_partition {
  /// Node schedule: gives order of node execution within each graph iteration
  std::vector<sdf_node_common*> _v_node_schedule;

  /// Index to a partition within a graph's vector of partitions:
  ///   graph->_v_partitions.
  std::size_t _part_index;

  /// Allows multiple contexts to be maintained to allow
  /// pipelining of the node schedule within a partition.
  std::list<partition_iteration> _resume_list;

  /// collected statistics
  std::chrono::time_point<std::chrono::system_clock> _execution_start_time;
  std::chrono::time_point<std::chrono::system_clock> _partition_resume_time;

  bool run_iter_schedule(
    partition_iteration& resume_partition_iteration,
    std::size_t          end_node_index,
    /// See sdfgraph.hh documentation on _interrupt_immediately
    volatile bool*       p_interrupt_immediately);

  bool run_partial_schedule(
    std::list<partition_iteration>::iterator it_rpi,
    bool           has_partial_schedule,
    bool           it_rpi_has_predecessor,
    bool           step_run_infinite_iterations,
    /// step_num_iterations defined iff step_run_infinite_iterations == false
    std::size_t    step_num_iterations,
    /// See sdfgraph.hh documentation on _interrupt_immediately
    volatile bool* p_interrupt_immediately);

  bool run_resume_list(
    bool           step_run_infinite_iterations,
    /// step_num_iterations defined iff step_run_infinite_iterations == false
    std::size_t    step_num_iterations,
    /// See sdfgraph.hh documentation on _interrupt_immediately
    volatile bool* p_interrupt_immediately,
    /// *p_control_mutex must be locked by partition to set
    /// *p_was_interrupted = true
    std::mutex*    p_control_mutex,
    bool*          p_was_interrupted);

public:
  /// API to run the partition. Partition starts execution from the
  /// resume information in _resume_list, initially a single resume
  /// point at the start of the graph execution. _resume_list holds
  /// information so that the partition can seamlessly start execution
  /// from where it stopped or was interrupted last time.
  ///
  /// p_interrupt_immediately, p_control_mutex and p_was_interrupted are
  /// internal fields of the sdf_graph executing on this partition.
  ///
  /// p_interrupt_immediately is used by graph to signal the partition
  /// to stop execution ASAP.
  /// If the partition detects *p_interrupt_immediately == true,
  /// it signals interruption by setting *p_was_interrupted = true.
  void run_partition(
    std::chrono::time_point<std::chrono::system_clock> const&
                   execution_start_time,
    bool           step_run_infinite_iterations,
    /// step_num_iterations defined iff step_run_infinite_iterations == false
    std::size_t    step_num_iterations,
    /// See sdfgraph.hh documentation on _interrupt_immediately
    volatile bool* p_interrupt_immediately,
    /// *p_control_mutex must be locked by partition to set
    /// *p_was_interrupted = true
    std::mutex*    p_control_mutex,
    bool*          p_was_interrupted);

  /// part_index is an integer ID for the partition, unique among
  /// the partitions created to execute a given graph.
  /// Conventionally the index of this partition in a vector
  /// of partition pointers maintained by sdf_graph.
  sdf_partition(std::size_t part_index);

  inline std::vector<sdf_node_common*>& access_v_node_schedule()
  {
    return _v_node_schedule;
  }

  inline std::vector<sdf_node_common*> const& get_v_node_schedule() const
  {
    return _v_node_schedule;
  }

  std::size_t get_part_index() const
  {
    return _part_index;
  }

  inline std::list<partition_iteration> const& get_resume_list() const
  {
    return _resume_list;
  }
};


  /// Determines the earliest and latest resume points
  /// across a given collection of partitions
  /// (intended to be all the partitions executing a graph)

void get_min_max_resume_piter(
                              std::vector<sdf_partition*> const& v_partitions,
                              partition_iteration& min_resume_piter,
                              partition_iteration& max_resume_piter);

  /// When a node is interrupted while executing by a blocked in- or
  /// out-channel, or by the assertion of p_interrupt_immediately, the
  /// partially executed node has to be re-executed later to complete its
  /// work for that given graph iteration. Channel pops and pushes that
  /// already completed before the interruption will not be re-done.
  /// Only the channels that failed to pop or push will be executed,
  /// and also the node-function if it is was not previously executed
  /// before the node was interrupted.
  ///
  /// The setup_channel_resumes() call sets a flag across all the channels so
  /// that the channels know a re-execution is going to take place when the
  /// node's iter_work() method is next invoked. This allows already executed
  /// channels to avoid repeting their pop/push. The node's iter_work() method
  /// automatically keeps track of whether the node was executed or not.
  /// However, to keep class sdf_graph a black-box from sdf_node_common,
  /// we cannot invoke setup_channel_resumes() from within the node's
  /// iter_work(), and instead the sdf_partition must invoke
  /// setup_channel_resumes() when it detects a partially executed node.
  ///
  /// Wrapper to sdf_graph::setup_channel_resumes().
void setup_channel_resumes(sdf_node_common* n);

} //namespace internal
} //namespace mare
