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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cassert>
#include <numeric>
#include <vector>

#include <mare/internal/debug.hh>
#include <mare/internal/sdf/sdfdebug.hh>
#include <mare/mare.h>
#include <mare/sdf.hh>

/////////////////////
//
// Multiple parallel pipelines
//
//  source --> f1 --> f2 --> sink
//         |-> f1 --> f2 -|
//         |-> f1 --> f2 -|
//
// Goal is to demonstrate:
//     Use of the sdfdebug.hh API which allows user to manually dictate how
//     the SDF graph gets partitioned across the cores. The user should
//     typically have no need to manually partition the graph, and should
//     instead rely on the MARE runtime to determine an appropriate
//     partitioning automatically based on available number of cores and costs
//     assigned by the user to the nodes (cost assignment is also optional,
//     default cost = 1.0 assumed for nodes where the user has not specified
//     a cost).
//
// This example builds on examples/sdfadvanced.cc. Please see that example
// first.
//
// Please also see examples/sdfassigncost.cc for how the user can assign
// node costs to guide even partitioning of the graph across CPU cores.
//
// The user should provide node costs and allow MARE to partition
// automatically. Use the explicit manual partitioning for debug purposes only.



const int num_parallel_pipelines = 3;

const int total_tokens = 2000;

struct token {
  bool _valid;
  int  _pipe_id;
  token() :
    _valid(false),
    _pipe_id(-1) {}

  // in general: add pointer to data payload here
};

std::vector<int> f1_valid_counts;
std::vector<int> f2_valid_counts;

std::vector<int> f1_invoke_counts;
std::vector<int> f2_invoke_counts;

void
f1(token& in_t, token& out_t)
{
  out_t = in_t;
  if(in_t._valid)
    f1_valid_counts[in_t._pipe_id]++;

  f1_invoke_counts[in_t._pipe_id]++;

  // in general: process data payload here
}

void
f2(token& in_t, token& out_t)
{
  out_t = in_t;
  if(in_t._valid)
    f2_valid_counts[in_t._pipe_id]++;

  f2_invoke_counts[in_t._pipe_id]++;

  // in general: process data payload here
}

mare::sdf_graph_ptr graph = 0;
int source_invoke_count = 0;

void
source(mare::node_channels& ncs)
{
  static int num_sent_tokens = 0;
  for(std::size_t i=0; i<ncs.get_num_channels(); i++) {
    token t;
    t._pipe_id = int(i);
    if(num_sent_tokens < total_tokens) {
      t._valid = true;
      num_sent_tokens++;
    }
    ncs.write(t, i);
  }
  if(num_sent_tokens == total_tokens) {
    MARE_LLOG("CANCELLING");
    mare::cancel(graph, mare::sdf_interrupt_type::iter_synced);
  }
  source_invoke_count++;
}

int num_sunk_tokens = 0;
int sink_invoke_count = 0;

void
sink(mare::node_channels& ncs)
{
  for(std::size_t i=0; i<ncs.get_num_channels(); i++) {
    token t;
    ncs.read(t, i);
    if(t._valid)
      num_sunk_tokens++;
  }
  sink_invoke_count++;
}


std::vector< mare::data_channel<token> * > vp_token_channels;

// In this example, v_nodes keeps the handles for all the
// created nodes. The handles are used later for manually
// assigning nodes to partitions.
// NOTE: manual assignment of nodes to partitions is not something
// the user is expected to do, but is sometimes useful for low-level
// debugging.
std::vector< mare::sdf_node_ptr > v_nodes;

typedef std::vector<mare::tuple_dir_channel> vec_channels;


void
create_pipeline(
                vec_channels& vec_channels_source,
                vec_channels& vec_channels_sink
)
{
  auto pdc1 = new mare::data_channel<token>;
  vp_token_channels.push_back(pdc1);
  auto pdc2 = new mare::data_channel<token>;
  vp_token_channels.push_back(pdc2);
  auto pdc3 = new mare::data_channel<token>;
  vp_token_channels.push_back(pdc3);

  vec_channels_source.push_back( mare::as_out_channel_tuple(*pdc1) );

  // save the handles for the created nodes.
  v_nodes.push_back( mare::create_sdf_node(graph, f1,
                                           mare::with_inputs(*pdc1),
                                           mare::with_outputs(*pdc2)) );
  v_nodes.push_back( mare::create_sdf_node(graph, f2,
                                           mare::with_inputs(*pdc2),
                                           mare::with_outputs(*pdc3)) );

  vec_channels_sink.push_back( mare::as_in_channel_tuple(*pdc3) );
}

int main() {
  mare::runtime::init();

  MARE_LLOG("num_parallel_pipelines = %d", num_parallel_pipelines);
  MARE_LLOG("total_tokens = %d", total_tokens);

  f1_valid_counts.resize(num_parallel_pipelines, 0);
  f2_valid_counts.resize(num_parallel_pipelines, 0);

  f1_invoke_counts.resize(num_parallel_pipelines, 0);
  f2_invoke_counts.resize(num_parallel_pipelines, 0);

  vec_channels vec_channels_source;

  vec_channels vec_channels_sink;

  graph = mare::create_sdf_graph();

  for(int i=0; i<num_parallel_pipelines; i++)
    create_pipeline(vec_channels_source, vec_channels_sink);

  // save the handles for the created nodes.
  v_nodes.push_back( mare::create_sdf_node(graph, source,
                                           vec_channels_source) );

  v_nodes.push_back( mare::create_sdf_node(graph, sink,
                                           vec_channels_sink) );

  // -- User-defined partition schedules -- //

  // First, set number of partitions
  mare::internal::set_sdf_num_partitions(graph, 5);

  // Next, assign nodes to partitions

  //partition 0: source node
  mare::internal::set_sdf_node_partition(v_nodes[6], 0);

  //partition 1: nodes in pipeline 1
  mare::internal::set_sdf_node_partition(v_nodes[0], 1);
  mare::internal::set_sdf_node_partition(v_nodes[1], 1);

  //partition 2: nodes in pipeline 2
  mare::internal::set_sdf_node_partition(v_nodes[2], 2);
  mare::internal::set_sdf_node_partition(v_nodes[3], 2);

  //partition 3: nodes in pipeline 3
  mare::internal::set_sdf_node_partition(v_nodes[4], 3);
  mare::internal::set_sdf_node_partition(v_nodes[5], 3);

  //partition 4: sink node
  mare::internal::set_sdf_node_partition(v_nodes[7], 4);

  // ---- //

  mare::launch_and_wait(graph);

  for(std::size_t i=0; i<vp_token_channels.size(); i++)
    delete vp_token_channels[i];

  int f1_token_sum = std::accumulate(f1_valid_counts.begin(),
                                     f1_valid_counts.end(),
                                     0);
  int f2_token_sum = std::accumulate(f2_valid_counts.begin(),
                                     f2_valid_counts.end(),
                                     0);

  MARE_LLOG("num_sunk_tokens = %d", num_sunk_tokens);
  MARE_LLOG("f1_token_sum = %d", f1_token_sum);
  MARE_LLOG("f2_token_sum = %d", f2_token_sum);

  assert(num_sunk_tokens == total_tokens);
  assert(f1_token_sum == total_tokens);
  assert(f2_token_sum == total_tokens);

  MARE_LLOG("source_invoke_count=%d", source_invoke_count);
  MARE_LLOG("sink_invoke_count  =%d", sink_invoke_count);
  for(int i=0; i<num_parallel_pipelines; i++) {
    MARE_LLOG("f1_invoke_counts[%d]=%d", i, f1_invoke_counts[i]);
    MARE_LLOG("f2_invoke_counts[%d]=%d", i, f2_invoke_counts[i]);
  }

  assert(source_invoke_count == sink_invoke_count);
  for(int i=0; i<num_parallel_pipelines; i++) {
    assert(source_invoke_count == f1_invoke_counts[i]);
    assert(source_invoke_count == f2_invoke_counts[i]);
  }

  mare::destroy_sdf_graph(graph);

  mare::runtime::shutdown();

  return 0;
}
