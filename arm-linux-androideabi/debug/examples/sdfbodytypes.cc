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

// Workaround for some configurations of gcc not
// enabling sleep_for() by default
#define _GLIBCXX_USE_NANOSLEEP

#include <mutex>
#include <thread>

#include <mare/mare.h>
#include <mare/sdf.hh>

///////////////
//
//  Pipeline
//     f1 -> (dc1) -> f2 -> (dc2) -> f3 -> (dc3) -> f4
//
//  Goal is to
//  1. Contrast the different styles of node-function bodies.
//  2. Illustrate some interesting interactions between
//     the application thread and graph nodes, made possible
//     by bodies carrying state.
//
//  For more involved illustration of each style see:
//   - sdfbasicpipe.cc:
//         regular C function body
//   - sdfprogrammatic.cc:
//         non-templated programmatic introspection body
//   - sdffiltergeneralized.cc:
//         lambdas and callable objects as bodies
//         (both allow a node-function to hold state across
//          invocations, and also allow manipulation of the
//          state from outside the graph).


// class payload allows the application to periodically
// query statistics stored in a node. If the statistics
// indicate some alarm condition, the app thread can
// pause the graph and reset some parameters.
class payload {
  // statistics would be more complex in general
  typedef long long int statistics;

  statistics _stats;

  // to guard the update of _stats and its
  // reading by the application thread
  std::mutex _mutex;

  long long int _seed;

public:
  payload(long long int seed = 0) :
    _stats(0),
    _mutex(),
    _seed(seed)
    {}

  // used by application thread to query statistics
  // while the graph is running
  statistics get_stats()
  {
    std::unique_lock<std::mutex> l;
    return _stats;
  }

  // application can pause the graph and reset seed
  void reset_seed()
  {
    std::unique_lock<std::mutex> l;
    _stats = 0;
    _seed = 0;
  }

  void do_node_work(int& output)
  {
    if(_seed > 100) // a "threshold exceeded" alarm
    {
      // capture alarm condition
      std::unique_lock<std::mutex> l;
      _stats++;

      // attempt to dissipate alarm condition temporarily
      _seed /= 5;
    }

    _seed = _seed * 3 + 1;
    output = int(_seed);
  }
};

// f1 = will be an instance of class source_node
//
// Carries a pointer to class payload
class source_node {
  payload* _p_payload;
public:
  source_node(payload* p_payload) :
    _p_payload(p_payload) {}

  // method called by MARE SDF to execute the node
  void operator()(int& output)
  {
    _p_payload->do_node_work(output);
  }
};

// f2 = non-templated programmatic introspection body
//
// Introspects the connected channels.
// Sums all the popped input values and replicates sum
// on all the output values to be pushed.
void f2(mare::node_channels& ncs) {
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


// f3 = lambda with capture.
// lambda body in main()

// f4 = regular C function
//
// Pretend to write pipeline outputs to a file
const char * filename = "xyz.txt";
bool is_file_open = false;

void f4(int& /*pipeline_result*/)
{
  if(!is_file_open) {
    // open file in filename for writing here
    is_file_open = true;
  }
  // write pipeline_result value to file here
}

int main() {
  // Initialize the MARE runtime.
  mare::runtime::init();

  mare::data_channel<int>  dc1, dc2, dc3;

  mare::sdf_graph_ptr g = mare::create_sdf_graph();

  payload p(45);
  // f1 = callable object
  mare::create_sdf_node(g, source_node(&p),
                        mare::with_outputs(dc1));

  // f2 = programmatic introspection
  mare::create_sdf_node(g, f2,
                        mare::with_inputs(dc1),
                        mare::with_outputs(dc2));
  int offset = 4;
  // f3 = lambda with capture
  mare::create_sdf_node(g,
                        [offset](int& in_val, int& out_val)
                        {
                          out_val = in_val + offset;
                        },
                        mare::with_inputs(dc2),
                        mare::with_outputs(dc3));

  // f4 = regular C function
  mare::create_sdf_node(g, f4,
                        mare::with_inputs(dc3));



  const std::size_t num_iterations = 10000;
  mare::launch(g, num_iterations);

  // Application thread executes concurrent to graph.
  // App thread periodically monitors graph's statistics.
  // If excessive alarm conditions are detected, the app
  // thread pauses the graph and resets parameters.
  while(mare::sdf_graph_query(g).has_completed() == false) {
    // periodically sample the graph statistics
    std::chrono::milliseconds dura(100);
    std::this_thread::sleep_for(dura);

    // check alarms raised so far
    long long int num_alarms = p.get_stats();
    if(num_alarms > 5)
    {
      mare::pause(g);
      p.reset_seed();
      mare::resume(g);
    }
  }

  // don't need mare::wait_for(g) as graph would have to complete
  // for the while loop above to end

  mare::destroy_sdf_graph(g);

  // Shutdown the MARE runtime.
  mare::runtime::shutdown();

  return 0;
}
