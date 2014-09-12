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
//  Pipeline
//     f1 -> (dc1) -> f2 -> (dc2) -> f3
//
//  Goal is to demonstrate how the graph execution can be
//  paused/resumed or cancelled at specific graph iterations
//  by the application thread.
//
//  Please first see examples/sdfasynclaunch.cc for illustration
//  of the asynchronous mare::launch() and mare::wait().


int f1_iter = -1;
void f1(int &) {
  f1_iter++;
}

int f2_iter = -1;
void f2(int& , float& ) {
  f2_iter++;
}

int f3_iter = -1;
void f3(float &) {
  f3_iter++;
}

int main() {
  // Initialize the MARE runtime.
  mare::runtime::init();

  const std::size_t num_iterations = 1000;

  mare::data_channel<int> dc1;
  mare::data_channel<float> dc2;

  auto g = mare::create_sdf_graph();
  mare::create_sdf_node(g, f1,
                        mare::with_outputs(dc1));
  mare::create_sdf_node(g, f2,
                        mare::with_inputs(dc1),
                        mare::with_outputs(dc2));
  mare::create_sdf_node(g, f3,
                        mare::with_inputs(dc2));

  // We want the graph to pause after completing iteration 250,
  // pause again after completing iteration 500, and
  // stop execution after completing iteration 750.
  std::size_t desired_pause_iter1 = 250;
  std::size_t desired_pause_iter2 = 500;
  std::size_t desired_cancel_iter = 750;

  // Initially launch for an infinite number of iterations.
  // The cancel() API will have to used to terminate the graph.
  mare::launch(g, num_iterations);
  MARE_LLOG("After launch: %s",
            mare::to_string(mare::sdf_graph_query(g)).c_str());

  // The following pause(), resume(), cancel() and wait_for() calls
  // could have been put in completely different application threads.

  // mare::pause() will block until graph completes iteration 250,
  // then returns control to the application thread. At this point
  // the application thread could potentially examine the current
  // results of the graph execution, and perhaps modify filter
  // coefficients used by a graph node, before resuming the graph.
  mare::pause(g, desired_pause_iter1, mare::sdf_interrupt_type::iter_synced);
  MARE_LLOG("Pause @%llu: f1_iter=%d f2_iter=%d f3_iter=%d",
            static_cast<unsigned long long>(desired_pause_iter1),
            f1_iter, f2_iter, f3_iter);
  MARE_LLOG("%s", mare::to_string(mare::sdf_graph_query(g)).c_str());
  // Graph execution is resumed by the mare::resume() API call.
  mare::resume(g);

  // This pause will block until the graph completes iteration 500.
  // Note the mare::sdf_graph_query() call that allows the application thread
  // to determine the current execution status of the graph, in this case
  // indicating that the graph is paused at iteration 500.
  mare::pause(g, desired_pause_iter2 , mare::sdf_interrupt_type::iter_synced);
  MARE_LLOG("Pause @%llu: f1_iter=%d f2_iter=%d f3_iter=%d",
            static_cast<unsigned long long>(desired_pause_iter2),
            f1_iter, f2_iter, f3_iter);
  MARE_LLOG("%s", mare::to_string(mare::sdf_graph_query(g)).c_str());
  mare::resume(g);

  // Cancels the graph execution after iteration 750. Does not
  // block for the cancel to take effect (i.e., for the graph
  // to stop execution).
  mare::cancel(g, desired_cancel_iter);

  // Following will have no effect, as graph cancel would take
  // effect prior to the pause. Once the graph cancels, the pause
  // will unblock. The subsequent resume has no effect.
  mare::pause(g);
  mare::resume(g);

  // Will not block, as the graph is guaranteed to have cancelled when
  // the last pause unblocked.
  mare::wait_for(g);
  MARE_LLOG("After wait_for: %s",
            mare::to_string(mare::sdf_graph_query(g)).c_str());

  MARE_LLOG("stopped after f1_iter = %d", f1_iter);
  MARE_LLOG("stopped after f2_iter = %d", f2_iter);
  MARE_LLOG("stopped after f3_iter = %d", f3_iter);

  mare::destroy_sdf_graph(g);

  // Shutdown the MARE runtime.
  mare::runtime::shutdown();

  return 0;
}
