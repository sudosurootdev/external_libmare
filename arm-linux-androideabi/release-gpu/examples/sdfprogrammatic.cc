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
//  Examples of programmatic construction of the graph structure
//  and introspection by node-functions to determine what channels
//  connect to them.
//
//     tf1 -> (dc1) -> pf2 -> (dc2) -> pf3
//         \           ^                ^
//          \          |                |
//          (dc5)      |                |
//             \       |                |
//            - \ -----|                |
//           /   \                      |
//        (dc6)   \                     |
//         /        >                   |
//     pf4 -> (dc3) -> tf5 -> (dc4) ----|
//
// Please first see examples/sdfbasicpipe.cc for the more
// basic mechanisms to construct a graph.

void tf1(int &out1, int& out2) {
  static int x = 0;
  out1 = x;
  out2 = x;
  x++;
}

// function with fixed non-templated signature,
// suitable for programmatically connecting with arbitrary channels
void pf2(mare::node_channels& ncs) {
  // pf2 sums the values on all the in-channels, then outputs
  // the sum on all out-channels.
  // Demonstrates introspection by a node function, as user-code
  // determines what in- and out-channels are connected to the node.
  //
  // Note that in-channels are already popped before the
  // the node-function is called, and the out-channels will be
  // pushed automatically after the node-function completes.
  // The read() and write() do not pop/push channels, but merely
  // access the storage for the previously popped and the yet to be
  // pushed data elements. Repeated invocations of read() or write()
  // on the same channel-index will only fetch the same previously
  // popped data and overwrite the same, as yet not pushed, storage,
  // respectively.

  int sum = 0;
  for(std::size_t i=0; i<ncs.get_num_channels(); i++) {
    if(ncs.is_in_channel(i)) {
      int in_data;
      ncs.read(in_data, i);
      sum += in_data;
    }
  }

  for(std::size_t i=0; i<ncs.get_num_channels(); i++) {
    if(ncs.is_out_channel(i)) {
      ncs.write(sum, i);
    }
  }
}

std::deque<int> d3;

// function with fixed non-templated signature,
// suitable for programmatically connecting with arbitrary channels
void pf3(mare::node_channels& ncs) {
  // Here we assume that the programmer already knows that channel-indices
  // 0 and 1 are in-channels and channel-index 2 is an out-channel, so
  // introspection is skipped.
  int in1, in2;
  ncs.read(in1, 0);
  ncs.read(in2, 1);
  d3.push_back(in1+in2);
}

// function with fixed non-templated signature,
// suitable for programmatically connecting with arbitrary channels
void pf4(mare::node_channels& ncs) {
  static int x = 0;
  ncs.write(x, 0);
  ncs.write(x, 1);

  // Uncommenting following line will trigger runtime error due to access
  //    with incorrect direction
  // ncs.read(x, 1);

  // Uncommenting following line will trigger runtime error due to exceeding
  //    number of connected channels
  // ncs.write(x, 2);

  x++;
}

void tf5(int &in1, int &in2, int &out) {
  out = in1 + in2;
}



int main() {
  // Initialize the MARE runtime.
  mare::runtime::init();

  const std::size_t num_iterations = 10;

  mare::data_channel<int> dc1, dc2, dc3, dc4, dc5, dc6;

  mare::sdf_graph_ptr g = mare::create_sdf_graph();

  // as usual, a regular node-function tf1 can be combined with a parameter
  // list of channels templated on the user-data-types they carry
  mare::create_sdf_node(g, tf1,
                        mare::with_outputs(dc1, dc5));

  // programmatic nodes pf2 and pf3 can be connected with a parameter list of
  // channels templated on the user-data-types they carry
  mare::create_sdf_node(g, pf2,
                        mare::with_inputs(dc1, dc6),
                        mare::with_outputs(dc2));
  mare::create_sdf_node(g, pf3,
                        mare::with_inputs(dc2, dc4));

  // a programmatic node pf4 can be connected to a programmatic
  // vector of channel-bindings (not templated on user-data-types)
  std::vector<mare::tuple_dir_channel> v_dir_channels4;
  v_dir_channels4.push_back( mare::as_out_channel_tuple(dc6) );
  v_dir_channels4.push_back( mare::as_out_channel_tuple(dc3) );
  mare::create_sdf_node(g, pf4, v_dir_channels4);
  // v_dir_channels4 has no type information that the connected
  // channels carry values of type int, or are of the same or
  // different data-types.

  // a regular node-function tf5 can be connected to a programmatic
  // vector of channel-bindings (not templated on user-data-types)
  std::vector<mare::tuple_dir_channel> v_dir_channels5;
  v_dir_channels5.push_back( mare::as_in_channel_tuple(dc3) );
  v_dir_channels5.push_back( mare::as_in_channel_tuple(dc5) );
  v_dir_channels5.push_back( mare::as_out_channel_tuple(dc4) );
  mare::create_sdf_node(g, tf5, v_dir_channels5);

  mare::launch_and_wait(g, num_iterations);

  MARE_LLOG("Result = ");
  for(auto i : d3)
      MARE_LLOG("%d ", i);

  mare::destroy_sdf_graph(g);

  // Shutdown the MARE runtime.
  mare::runtime::shutdown();

  return 0;
}
