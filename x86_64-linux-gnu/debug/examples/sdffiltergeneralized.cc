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
// Demonstrates that MARE SDF allows the user to use general-purpose,
// highly re-usable components. The components are likely to be in
// a form that is useful outside MARE SDF, and are likely to already
// exist in user libraries (e.g., the adder and splitter node-functions
// below). The use of variadic templates in creating the node-functions
// grants the node-functions further flexibility to connect to a variety
// of SDF graph structures. The use of callable objects allows state to
// be maintained in a node-function (e.g., class mult below).
//
// This example implements a filter identical to examples/sdffilter.cc,
// but using highly re-usable components.
// Please see examples/sdffilter.cc first.
//
// Filter
//     xi --> add1 --> mult --> add2 --> yi
//             ^                 ^    |
//             |                 |--D-|
//             |                      |
//             |-----D------D---------|


void gen_xi(float &result) {
  static float x = 0;
  result = x++;
}

// void adder(Vs&... vs, R& sum):
// add an arbitrary number of inputs of types compatible for addition
template<typename V, typename R>
void adder(V& v, R& sum) {
  sum = v;
}

template<typename V, typename R, typename ...Vs>
void adder(Vs&... vs, V& v, R& sum) {
  R tmp;
  adder(vs..., tmp);
  sum = tmp + v;
}

// class mult: scale input by a pre-set coefficient.
//
// A node-function implemented as an instance of class mult
// holds a coefficient value as node-function state.
// An executing graph could be paused after a few iterations,
// and the coefficient value changed before resuming graph
// execution (see examples/sdfpauseresumecancel.cc).
template<typename C, typename V = C, typename R = C>
class mult {
  C _coeff;
public:
  mult(C const& coeff)
    : _coeff(coeff) {}

  void operator()(V& in, R& prod)
  {
    prod = _coeff * in;
  }
};

// void splitter(V& v, Rs&... rs):
// split an input value into an arbitary number of outputs of compatible types
template<typename V, typename R>
void splitter(V& v, R& r) {
  r = v;
}

template<typename V, typename R, typename ...Rs>
void splitter(V& v, R& r, Rs&... rs) {
  r = v;
  splitter(v, rs...);
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

  // Connect adder to any number of input channels and one
  // output channel.
  mare::create_sdf_node(g,
                        [](float& in1, float& in2, float& sum)
                        { // explicit wrapping in lambda to
                          // circumvent a clang compiler limitation
                          // (not needed with gcc or VS2013)
                          adder<float, float, float>(in1, in2, sum);
                        },
                        mare::with_inputs(dc1, dd_dc),
                        mare::with_outputs(dc2));

  // Node function that maintains state.
  mare::create_sdf_node(g, mult<float>(0.3f), // coefficient for scaling
                        mare::with_inputs(dc2),
                        mare::with_outputs(dc3));

  mare::create_sdf_node(g,
                        [](float& in1, float& in2, float& sum)
                        { // explicit wrapping in lambda to
                          // circumvent a clang compiler limitation
                          // (not needed with gcc or VS2013)
                          adder<float, float, float>(in1, in2, sum);
                        },
                        mare::with_inputs(dc3, d_dc),
                        mare::with_outputs(dc4));

  // Connect splitter to one input and any number of output channels
  mare::create_sdf_node(g, splitter<float, float, float, float>,
                        mare::with_inputs(dc4),
                        mare::with_outputs(dc5, dd_dc, d_dc));

  mare::create_sdf_node(g, save_yi,
                        mare::with_inputs(dc5));

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
