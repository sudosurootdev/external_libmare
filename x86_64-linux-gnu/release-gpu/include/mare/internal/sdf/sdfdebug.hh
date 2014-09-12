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

#include <mare/internal/sdf/sdfbasedefs.hh>

namespace mare {
namespace internal {

//////////////
// Debug Interface

  /// Sets number of partitions for the SDF graph g to use
  ///
  /// Impacts automatic partitioning, if used.
  /// Necessary to explicitly set num_partitions beforehand
  /// if manual partitioning is done with set_sdf_node_partition()
void set_sdf_num_partitions(sdf_graph_ptr g, std::size_t num_partitions);

  /// Appends a node to a partition's node-schedule.
  ///
  /// If used at all, must be used to assign partitions to all nodes in the
  /// graph. Automatic partitioning gets disabled.
  ///
  /// set_sdf_node_partition() must be invoked prior to the first call to
  /// set_sdf_node_partition.
  ///
  /// Nodes within each partition are scheduled in the order of their
  ///   set_sdf_node_partition() call.
  ///
  /// CAUTION: a schedule must respect the following properties to be a valid.
  ///  An invalid schedule can cause execution to hang on inter-partition
  ///  channel read/writes, or worse cause node-functions to read invalid
  ///  input values.
  ///
  ///  1. Nodes within each partition must be ordered to respect dependences
  ///    (ignoring pre-loaded channels)
  ///  2. Nodes across partitions should respect dependences (ignoring
  ///    pre-loaded channels), such that a successor node does not occur
  ///    in a partition-index lower than the partition-index of any
  ///    predecessor node.
void set_sdf_node_partition(sdf_node_ptr n, std::size_t partition_index);

} //namespace internal
} //namespace mare
