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

#include <list>
#include <string>

namespace mare {
namespace internal {

  /// An SDF Partition executes a topologically sorted subset of the SDF graph
  /// nodes.
  /// A partition needs a "resume" point to start execution of its nodes, and
  /// an "end" point (possibly infinity). The execution must stop just *before*
  /// the end point, but start at the resume point. Further, if the execution
  /// of a partition is interrupted by a signal from the SDF launcher, the
  /// SDF launcher needs to determine a new resume point for the partition, so
  /// that the combined execution prior to and subsequent to the interruption
  /// appears "seamless". Therefore, the end point of an interrupted execution
  /// is defined as the next resume point when (if) the execution is re-tried
  /// by the SDF launcher.
  ///
  /// struct partition_iteration: representation of the resume point.
  ///
  /// Information requirements for partition_iteration:

  /// - Execution can be interrupted in the middle of a graph iteration. Hence,
  /// the next node to execute is noted in the resume information.
  ///
  /// - Further, a node may only "partially" execute if a blocked channel is
  /// encountered or the SDF launcher's interruption signal is detected in
  /// between a node's pop channels, apply node-function, and push channels
  /// stages. Hence, a flag must also indicate if a node was interrupted so
  /// that the node gets re-tried when execution is resumed. The node
  /// execution logic (see sdf_node_typed::iter_work() in
  /// sdfapiimplementation.hh), internally keeps track of how far the node
  /// executed, so the resume point does not need to hold those details.
  ///
  /// The partition_iteration consists of the following fields to accommodate
  /// the resume information requirements discussed above.
  ///  - _iter      : the overall graph iteration
  ///
  ///  - _node_index: which node to resume execution at within _iter

  ///  - _before_first_iter: a special case to indicate that execution has
  ///         not even been attempted. Distinct from (_iter=0, _node_index=0).
  ///         Semantics: (_before_first_iter=true) < (_iter=0, _node_index=0),
  ///                    where "<" means "strictly earlier in execution order".
  ///
  ///  - _node_interrupted: flags that _node_index was partially executed,
  ///                       hence needs to be re-tried when excecution resumes.
  ///         Semantics: (_iter=x, _node_index=y, _node_interrupted=false) <
  ///                    (_iter=x, _node_index=y, _node_interrupted=true),
  ///                    as the latter has partially executed node y, while
  ///                    the former is about to begin execution of y.
  ///
  ///  The captured resume information allows the SDF launcher to compare the
  ///  progress of multiple partitions, and determine which partition is the
  ///  most ahead and which is the most behind (using comparison operators
  /// defined on partition_iteration). This information helps the SDF launcher
  /// to make determinations about the "consistency" of an interruption across
  ///  partitions, and selectively execute any lagging partitions to an
  ///  acceptable "synchronization point" consistent with the desired
  ///  interruption semantics.

struct partition_iteration {
  bool        _before_first_iter;
  std::size_t _iter;
  std::size_t _node_index;
  bool        _node_interrupted;

  /// resume at very start of execution
  partition_iteration() :
    _before_first_iter(true),
    _iter(0),
    _node_index(0),
    _node_interrupted(false) {}

  /// resume at start of graph iteration _iter
  partition_iteration(
    std::size_t iter
  ) :
    _before_first_iter(false),
    _iter(iter),
    _node_index(0),
    _node_interrupted(false) {}

  /// resume at node = (_iter, _node_index):
  ///     at start of node if node_interrupted == false,
  ///     otherwise, resume partially executed node
  partition_iteration(
    std::size_t iter,
    std::size_t node_index,
    bool        node_interrupted = false
  ) :
    _before_first_iter(false),
    _iter(iter),
    _node_index(node_index),
    _node_interrupted(node_interrupted) {}
};

inline bool operator<(partition_iteration const& piter1,
                      partition_iteration const& piter2)
{
  if(piter1._before_first_iter && !piter2._before_first_iter)
    return true;
  if(piter2._before_first_iter)
    return false;
  if(piter1._iter < piter2._iter)
    return true;
  if(piter1._iter > piter2._iter)
    return false;
  if(piter1._node_index < piter2._node_index)
    return true;
  if(piter1._node_index > piter2._node_index)
    return false;
  return (!piter1._node_interrupted && piter2._node_interrupted);
          /// comparing resume points
}

inline bool operator==(partition_iteration const& piter1,
                       partition_iteration const& piter2)
{
  return (piter1._before_first_iter == piter2._before_first_iter) &&
         (piter1._iter              == piter2._iter             ) &&
         (piter1._node_index        == piter2._node_index       ) &&
         (piter1._node_interrupted  == piter2._node_interrupted );
}

inline bool operator<=(partition_iteration const& piter1,
                       partition_iteration const& piter2)
{
  return (piter1 < piter2) || (piter1 == piter2);
}

inline bool operator>=(partition_iteration const& piter1,
                       partition_iteration const& piter2)
{
  return !(piter1 < piter2);
}

inline bool operator>(partition_iteration const& piter1,
                      partition_iteration const& piter2)
{
  return !(piter1 <= piter2);
}

std::string to_string(partition_iteration const& pi);

std::string to_string(std::list<partition_iteration> const& lpi);

} //namespace internal
} //namespace mare
