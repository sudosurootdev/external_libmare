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

#include <deque>

#include <mare/internal/debug.hh>
#include <mare/mare.h>
#include <mare/sdf.hh>

///////////////
//
//   Filter
//     xi --> add1 --> mult --> add2 --> yi
//             ^                 ^    |
//             |                 |--D-|
//             |                      |
//             |-----D------D---------|
//
// Goal is to illustrate creation of synchronous dataflow graphs
// with feedback edges. Any cycle in a synchronous dataflow graph
// must have at least one 'delay' on an edge constituting the
// cycle, otherwise it is not a valid synchronous dataflow graph.
//
// MARE SDF represents edges as mare::data_channel<T> objects templated by
// the user-data-type T carried by the channel.
//
// The mare::preload_channel() API call adds a desired number of 'delays'
// to a given channel, in the form of a sequence of values to initialize
// the channel with. At the start of the graph execution, a pre-loaded
// channel will already contain that sequence of values before any value
// has been pushed onto it by a node-function.
//
// Please first see examples/sdfbasicpipe.cc before proceeding
// with this example.

void gen_xi(float &result) {
  static float x = 0;
  result = x++;
}

void add1(float &in_xi, float &yi_DD, float &result) {
  result = in_xi + yi_DD;
}

void mult(float &in_val, float &result) {
  result = 0.3f * in_val;
}

void add2(float &in_val, float &yi_D, float &result) {
  result = in_val + yi_D;
}

void splitter(float &in_yi, float &out_yi, float &fb_yi1, float &fb_yi2) {
  out_yi = in_yi;
  fb_yi1 = in_yi;
  fb_yi2 = in_yi;
}

std::deque<float> dy;
void save_yi(float &yi) {
  dy.push_back(yi);
}


int main() {
  // Initialize the MARE runtime.
  mare::runtime::init();

  const std::size_t num_iterations = 10;

  mare::data_channel<float> dc1, dc2, dc3, dc4, dc5, d_dc, dd_dc;

  mare::sdf_graph_ptr g = mare::create_sdf_graph();

  mare::create_sdf_node(g, gen_xi,
                        mare::with_outputs(dc1));

  mare::create_sdf_node(g, add1,
                        mare::with_inputs(dc1, dd_dc),
                        mare::with_outputs(dc2));

  mare::create_sdf_node(g, mult,
                        mare::with_inputs(dc2),
                        mare::with_outputs(dc3));

  mare::create_sdf_node(g, add2,
                        mare::with_inputs(dc3, d_dc),
                        mare::with_outputs(dc4));

  mare::create_sdf_node(g, splitter,
                        mare::with_inputs(dc4),
                        mare::with_outputs(dc5, dd_dc, d_dc));

  mare::create_sdf_node(g, save_yi,
                        mare::with_inputs(dc5));

  // Feedback channels must have values pre-loaded.
  // Pre-load value sequence corresponds to number of delays
  // desired along that channel.

  // preload ==> Delay Delay
  std::vector<float> initial_values_dd_dc;
  initial_values_dd_dc.push_back(0.0);
  initial_values_dd_dc.push_back(0.0);
  mare::preload_channel(dd_dc, initial_values_dd_dc);

  // preload ==> Delay
  std::vector<float> initial_values_d_dc;
  initial_values_d_dc.push_back(0.0);
  mare::preload_channel(d_dc, initial_values_d_dc);

  mare::launch_and_wait(g, num_iterations);

  MARE_LLOG("Result = ");
  for(auto f : dy)
      MARE_LLOG("%f ", f);


  mare::destroy_sdf_graph(g);

  // Shutdown the MARE runtime.
  mare::runtime::shutdown();

  return 0;
}
