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
//  Goal is to demonstrate:
//  1. Creation of repetitive structures: in this example, user code in
//     create_pipeline() is invoked repeatedly to create multiple pipeline
//     structures within the graph
//  2. Cancel request issued from inside the graph, once a graph node
//     determines a termination criteria has been reached (in this example,
//     the source node terminates the graph when a certain number of input
//     tokens are processed)
//  3. Demonstrate use of tokens as streaming data to allow different parts
//     of the graph to execute a different number of "work" iterations despite
//     being part of the same SDF graph (see t._valid tests in the
//     user functions)
//
// Please first see examples/sdfprogrammatic.cc for programmatic construction
// of graphs.



// num_parallel_pipelines controls how many parallel pipelines should be
// created. Each pipeline is created by an invocation of create_pipeline()
const int num_parallel_pipelines = 3;

// total_tokens tells the source node how many tokens should be processed
// by the graph before the source node signals cancellation of the graph.
const int total_tokens = 2000;


// Convention for this example:
//   a token is valid iff _valid == true.
// Only valid tokens are counted towards total_tokens
// number of tokens being processed by the graph.
//
// In the last graph iteration, some pipelines will process a valid token,
// while some pipelines will pass an invalid token (on which no processing
// is done). This way num_parallel_pipelines does not have to uniformly
// divide total_tokens, yet precisely total_tokens amount of useful work
// is done by the graph.

struct token {
  bool _valid;
  int  _pipe_id;
  token() :
    _valid(false),
    _pipe_id(-1) {}

  // in general: add pointer to data payload here
};

// For demonstration purposes: used for correctness check on the
// number of valid tokens processed by each pipeline.
//
// f1_valid_counts[i] counts number of valid tokens
// processed by node f1 of the i'th pipeline.
// f2_valid_counts[i] serves a similar purpose for node f2 in the
// i'th pipeline.
std::vector<int> f1_valid_counts;
std::vector<int> f2_valid_counts;

// f1_invoke_counts[i] captures the number of times node f1 was
// invoked in the i'th pipeline, regardless of whether it received
// a valid token to process.
// f2_invoke_counts[i] has a similar purpose for f2 in the i'th pipeline.
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

mare::sdf_graph_ptr graph = nullptr;

// The source node executes mare::cancel() when a desired number of
// valid tokens (== total_tokens) have been sent into the pipelines.
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


// In this example, the channels will be dynamically allocated.
// vp_token_channels keeps track of all the allocated channels.
std::vector< mare::data_channel<token> * > vp_token_channels;


// A convenient typedef for holding a sequence of non-templated channels
// that will be programmatically connected to a node. Each element in the
// sequence is a channel-direction binding produced by a
// mare::as_in_channel_tuple() or mare::as_out_channel_tuple().
typedef std::vector<mare::tuple_dir_channel> vec_channels;


// User function to create a pipeline: including allocation of channels,
// creation of nodes, and making the connections between channels and
// node-functions. Appends the input channel to the pipeline into
// vec_channels_source and the output channel out of the pipeline
// into vec_channels_sink.
void
create_pipeline(
                vec_channels& vec_channels_source,
                vec_channels& vec_channels_sink
)
{
  // Dynamically allocate channels for the pipeline
  // and save into vp_token_channels.
  auto pdc1 = new mare::data_channel<token>;
  vp_token_channels.push_back(pdc1);
  auto pdc2 = new mare::data_channel<token>;
  vp_token_channels.push_back(pdc2);
  auto pdc3 = new mare::data_channel<token>;
  vp_token_channels.push_back(pdc3);

  vec_channels_source.push_back( mare::as_out_channel_tuple(*pdc1) );

  mare::create_sdf_node(graph, f1,
                        mare::with_inputs(*pdc1),
                        mare::with_outputs(*pdc2));
  mare::create_sdf_node(graph, f2,
                        mare::with_inputs(*pdc2),
                        mare::with_outputs(*pdc3));

  vec_channels_sink.push_back( mare::as_in_channel_tuple(*pdc3) );
}

int main() {
  // Initialize the MARE runtime.
  mare::runtime::init();

  MARE_LLOG("num_parallel_pipelines = %d", num_parallel_pipelines);
  MARE_LLOG("total_tokens = %d", total_tokens);

  f1_valid_counts.resize(num_parallel_pipelines, 0);
  f2_valid_counts.resize(num_parallel_pipelines, 0);

  f1_invoke_counts.resize(num_parallel_pipelines, 0);
  f2_invoke_counts.resize(num_parallel_pipelines, 0);

  // Will contain channels leaving the source node
  vec_channels vec_channels_source;

  // Will contain channels entering the sink node
  vec_channels vec_channels_sink;

  graph = mare::create_sdf_graph();

  // Create as many parallel pipes as desired, and append the
  // in- and out-channels for each pipeline into vec_channels_source
  // and vec_channels_sink, respectively.
  for(int i=0; i<num_parallel_pipelines; i++)
    create_pipeline(vec_channels_source, vec_channels_sink);

  // source node programmatically connects with channels in vec_channels_source
  mare::create_sdf_node(graph, source, vec_channels_source);

  // sink node programmatically connects with channels in vec_channels_sink
  mare::create_sdf_node(graph, sink, vec_channels_sink);

  mare::launch_and_wait(graph);

  // Explicitly delete the dynamically allocated channels, as
  // they will not be scope destructed.
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

  // For demo purposes, explicitly verify that nodes f1 (and similarly f2)
  // across pipelines cumulatively passed a number of valid tokens equal to
  // total_tokens. This is what is expected because the source node only feeds
  // total_tokens number of valid tokens into the pipelines collectively.
  assert(num_sunk_tokens == total_tokens);
  assert(f1_token_sum == total_tokens);
  assert(f2_token_sum == total_tokens);

  MARE_LLOG("source_invoke_count=%d", source_invoke_count);
  MARE_LLOG("sink_invoke_count  =%d", sink_invoke_count);
  for(int i=0; i<num_parallel_pipelines; i++) {
    MARE_LLOG("f1_invoke_counts[%d]=%d", i, f1_invoke_counts[i]);
    MARE_LLOG("f2_invoke_counts[%d]=%d", i, f2_invoke_counts[i]);
  }

  // For demo purposes, explicitly verify precision of the iter_synced cancel()
  // invoked by the source node, by matching invocation counts of every node.
  assert(source_invoke_count == sink_invoke_count);
  for(int i=0; i<num_parallel_pipelines; i++) {
    assert(source_invoke_count == f1_invoke_counts[i]);
    assert(source_invoke_count == f2_invoke_counts[i]);
  }

  mare::destroy_sdf_graph(graph);

  // Shutdown the MARE runtime.
  mare::runtime::shutdown();

  return 0;
}
