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

#include <mare/mare.h>
#include <mare/sdf.hh>

///////////////
//
//  Pipeline
//     myf1 -> (dc1) -> myf2 -> (dc2) -> myf3
//
//  Goal is to
//  1. illustrate construction of a very basic SDF graph,
//     consisting of a pipeline of nodes connected by channels, and
//  2. execute the graph for a number of iterations using the
//     blocking mare::launch_and_wait() API call.


// User functions for nodes

void myf1(int &a) {
  static int x = 0;
  a = x++;
}

void myf2(int& a, int& b) {
  b = a * 2;
}

// dq will hold the final results after the graph executes
std::deque<int> dq;
void myf3(int& b) {
  dq.push_back(b);
}

int main() {
  // Initialize the MARE runtime.
  mare::runtime::init();

  // Create the channels that will pass data between nodes.
  // Here both channels carry data of type int. In general,
  // a channel can carry data of any user-defined-type
  // that is copyable by memcpy().
  mare::data_channel<int>  dc1, dc2;

  // Create a new SDF graph
  mare::sdf_graph_ptr g = mare::create_sdf_graph();

  // Add nodes to the graph, indicating channel connections
  mare::create_sdf_node(g, myf1, mare::with_outputs(dc1));

  mare::create_sdf_node(g, myf2, mare::with_inputs(dc1),
                                 mare::with_outputs(dc2));
  // Note: User-function parameter-lists and the intended use of each
  // parameter as input or output must correspond in sequence to the
  // channels connected as inputs and outputs.
  //
  // Example: parameter list of myf2(int& a, int& b)
  // corresponds to the int and int data-types of the connected
  // channels dc1 and dc2. Parameter 'a' is used as input by
  // myf2 corresponding to the channel connection with_inputs(dc1).
  // Parameter 'b' is an output produced by myf2 and corresponds to
  // the with_outputs(dc2) channel connection.
  // The MARE runtime will pop a value from dc1 into storage
  // passed as 'a' before invoking myf2. After myf2 completes,
  // the value in the storage passed as 'b' will be pushed by
  // the MARE runtime into channel dc2.

  mare::create_sdf_node(g, myf3, mare::with_inputs(dc2));

  // Execute the graph for 10 iterations, block till completion
  const std::size_t num_iterations = 10;
  mare::launch_and_wait(g, num_iterations);

  // Now data structure dq contains 10 values

  // De-allocate the nodes and graph
  mare::destroy_sdf_graph(g);

  // Shutdown the MARE runtime.
  mare::runtime::shutdown();

  // Channels dc1 and dc2 will be scope-destructed
  return 0;
}
