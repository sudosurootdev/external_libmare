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

#include <mare/mare.h>
#include <mare/alignedallocator.hh>
#include <mare/patterns.hh>

//Create a page aligned allocator.
template<class T, size_t Alignment>
using aligned_vector = std::vector<T, mare::aligned_allocator<T, Alignment> >;
template<class T>
using page_aligned_vector = aligned_vector<T, 4096>;

//Create a string containing OpenCL C kernel code.
#define OCL_KERNEL(name, k) std::string const name##_string = #k

OCL_KERNEL(vadd_kernel,
  __kernel void vadd(__global float* A, __global float* B, __global float* C,
                     unsigned int size) {
    unsigned int i = get_global_id(0);
    if(i < size)
       C[i] = A[i] + B[i];
  });

int
main(void)
{
  //Initialize the MARE runtime.
  mare::runtime::init();

  //Create input vectors
  page_aligned_vector<float> a(1024);
  page_aligned_vector<float> b(a.size());

  //Initialize the input vectors
  for (size_t i = 0; i < a.size(); ++i) {
    a[i] = i;
    b[i] = a.size() - i;
  }

  //Create mare::buffer with initialization data.
  auto buf_a = mare::create_buffer(a.data(), a.size());
  auto buf_b = mare::create_buffer(b.data(), b.size());

  //Create mare::buffer with no host ptr & initialization data.
  auto buf_c = mare::create_buffer<float>(a.size());

  //Name of the OpenCL C kernel.
  std::string kernel_name("vadd");

  //Create a kernel object,
  auto gpu_vadd = mare::create_kernel<mare::buffer_ptr<const float>,
                                      mare::buffer_ptr<const float>,
                                      mare::buffer_ptr<mare::out<float>>,
                                      unsigned int>(vadd_kernel_string,
                                                    kernel_name);

  //Create a task attribute to mark the task as a gpu task.
  auto attrs = mare::create_task_attrs(mare::attr::gpu);

  unsigned int size = a.size();

  //Create a mare::range object, 1D in this case.
  mare::range<1> range_1d(a.size());

  //Create a ndrange_task
  auto gpu_task =
    mare::create_ndrange_task(range_1d,
                              mare::with_attrs(attrs, gpu_vadd,
                                               buf_a, buf_b, buf_c, size));


  //Launch the task
  mare::launch(gpu_task);

  //Wait for task completion.
  mare::wait_for(gpu_task);

  //compare the results.
  for(size_t i = 0; i < a.size(); ++i) {
    MARE_INTERNAL_ASSERT(a[i] + b[i] == buf_c[i] && buf_c[i] == a.size(),
                         "comparison failed at ix %zu: %f + %f == %f == %zu",
                         i, a[i], b[i], buf_c[i], a.size());
  }

  //shutdown the mare runtime
  mare::runtime::shutdown();
}

