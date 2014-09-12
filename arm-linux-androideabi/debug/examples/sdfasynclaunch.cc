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
//  Goal is to illustrate use of the asynchronous
//  launch API:
//    - mare::launch() performs non-blocking launch of execution
//    - mare::wait_for() allows user thread to wait for graph to
//      complete execution
//
//  Please first see examples/sdfbasicpipe.cc to understand
//  more basic concepts about construction of the graph and
//  the simpler blocking launch API.


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

  mare::data_channel<int>  dc1, dc2;

  mare::sdf_graph_ptr g = mare::create_sdf_graph();

  mare::create_sdf_node(g, myf1, mare::with_outputs(dc1));

  mare::create_sdf_node(g, myf2, mare::with_inputs(dc1),
                                 mare::with_outputs(dc2));

  mare::create_sdf_node(g, myf3, mare::with_inputs(dc2));

  // Launch execution of the graph for 10 iterations, does not block
  const std::size_t num_iterations = 10;
  mare::launch(g, num_iterations);

  // Graph may still be executing here, results is data-structure
  // dq may not be ready yet.

  // In general, the application thread can continue executing other
  // work here concurrent with the execution of the SDF graph

  // Block application thread till graph completes execution
  mare::wait_for(g);

  // Now data structure dq contains 10 values

  mare::destroy_sdf_graph(g);

  // Shutdown the MARE runtime.
  mare::runtime::shutdown();

  return 0;
}
