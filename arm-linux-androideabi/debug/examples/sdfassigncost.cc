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

#include <cmath>

#include <mare/internal/sdf/sdfdebug.hh>
#include <mare/mare.h>
#include <mare/sdf.hh>

/////////////////////
//
// Assignment of Relative Costs to graph nodes
//
// This version of MARE statically partitions the graph nodes across the
// available CPU cores. To achieve high performance and high utilization
// of the cores, the partitions need to be balanced. That is, the cumulative
// execution times of nodes within each partition should roughly be the
// same across partitions.
//
// The mare::assign_cost() API call allows the user to indicate the
// relative execution time of a node. A cost = 1.0 is assumed by
// default for all nodes. A user-specified cost of 'c' for node 'n'
// would indicate to MARE that 'n' has execution time 'c' times that
// of a node with cost 1.0.
//
// Please first see examples/sdfbasicpipe.cc for the basic mechanisms
// to construct a graph and execute it.

int main()
{
  mare::runtime::init();

  mare::sdf_graph_ptr g = mare::create_sdf_graph();

  mare::data_channel<float> dc1, dc2, dc3;

  int iter = 0;

  // We need to retain handles to the nodes created so we can later assign
  // costs to them. Handles n1, n2, n3, n4.

  // Node n1
  // (no handle retained, as we don't explicitly set a cost for n1)
  mare::create_sdf_node(g,
                                  [&iter](float& out)
                                  {
                                    out = float(iter++);
                                  },
                                  mare::with_outputs(dc1));

  auto n2 = mare::create_sdf_node(g,
                                  [](float& in, float &out)
                                  {
                                    // Time-consuming,
                                    // let's say roughly 10x compared to n1
                                    out = std::sqrt(in);
                                  },
                                  mare::with_inputs(dc1),
                                  mare::with_outputs(dc2));
  // n1 has an unspecified default cost of 1.0. We want n2 to be 10x of n1.
  mare::assign_cost(n2, 10.0);

  auto n3 = mare::create_sdf_node(g,
                                  [](float& in, float &out)
                                  {
                                    out = 2 * std::sqrt(in + 30.0f);
                                  },
                                  mare::with_inputs(dc2),
                                  mare::with_outputs(dc3));
  // Expect execution time of n3 to be similar to n2
  mare::assign_cost(n3, 10.0);

  float result = 0;
  auto n4 = mare::create_sdf_node(g,
                                  [&result](float& in)
                                  {
                                    float x = std::sqrt(in + 100.0f);
                                    float y = std::sqrt(x + 20.0f);
                                    result  = 0.5f * (result + y);
                                  },
                                  mare::with_inputs(dc3));
  // n4 appears to be twice as heavy as n2
  mare::assign_cost(n4, 20.0);

  // The launch will partition the nodes across partitions and attempt to
  // create roughly equal partitions. In this contrived example the graph
  // has only 4 nodes. Let's say MARE had two cores available to execute
  // the graph. Based on the costs assigned to the nodes, MARE would
  // partition the nodes as (n1, n2, n3) and (n4), and the two partitions
  // would get roughly equal cumulative costs of 21 and 20, respectively.
  // If no costs were assigned by the user, the default costs of 1.0 would
  // produce a partitioning of (n1, n2), (n3, n4), with cumulative costs of
  // 11 and 31, respectively, causing the second partition to become a
  // performance bottleneck while the first partition idled frequently.
  const std::size_t num_iterations = 10;
  mare::launch_and_wait(g, num_iterations);

  MARE_LLOG("result = %f", result);

  mare::destroy_sdf_graph(g);

  mare::runtime::shutdown();

  return 0;
}

