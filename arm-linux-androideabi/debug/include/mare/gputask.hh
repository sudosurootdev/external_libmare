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
#pragma once

#include <mare/group.hh>
#include <mare/patterns.hh>

namespace mare {

/** @cond HIDDEN */
template<size_t DIMS, typename Body, typename T>
struct ndrange_task_creator;

//specialization to create cpu ndrange task.
template<size_t DIMS, typename Body>
struct ndrange_task_creator<DIMS, Body, std::false_type>
{
  static task_ptr create(const mare::range<DIMS>& r,
                         Body&& body)
  {
    auto t = create_task([=]() {
               mare::pfor_each(r, body);
             });
    return t;
  }
};

#ifdef HAVE_OPENCL

namespace internal {
  // GPU devices on the platform.
  extern std::vector<mare::device_ptr> s_dev_ptrs;
}

//specialization to create gpu ndrange task.
template<size_t DIMS, typename Body>
struct ndrange_task_creator<DIMS, Body, std::true_type>
{
  static task_ptr create(const mare::range<DIMS>& r,
                         Body&& body)
  {

    typedef internal::body_wrapper<Body> wrapper;
    //get devices.
    MARE_INTERNAL_ASSERT(!internal::s_dev_ptrs.empty(),
                         "No GPU devices found on the platform");
    internal::gpudevice *d_ptr = internal::c_ptr(internal::s_dev_ptrs[0]);
    MARE_API_ASSERT((d_ptr != nullptr), "null device_ptr");
    internal::task* t = wrapper::create_task(internal::s_dev_ptrs[0],
                                             r,
                                             std::forward<Body>(body));

    return mare::task_ptr(t, mare::task_ptr::ref_policy::NO_INITIAL_REF);
  }
};

#endif // HAVE_OPENCL

/** @endcond */

/** @addtogroup tasks_creation
@{ */
/**
    Creates a new task and returns a task_ptr that points to that task.

    This template method creates a task and returns a pointer to it.
    The task executes the <tt>Body</tt> passed as parameter for each
    point in the iteration space of mare::range specified by the first
    argument. The preferred type of <tt>\<typename Body\></tt> is a
    C++11 lambda, although it is possible to use other types such as function
    objects and function pointers. Regardless of the <tt>Body</tt>
    type, it has to take an appropriate mare::index object as an argument.
    @sa mare::task(Body&&)

    @param r Range object (1D, 2D or 3D) representing the iteration space.
    @param body Code that the task will run when the tasks executes.

    @return
    task_ptr -- Task pointer that points to the new task.

    @par Example 1 Create a 1D range task using a C++11 lambda.
    @include snippets/create_1d_range.cc
    @include snippets/create_1d_range_task.cc

    @par Example 2 Create a 2D range task using a C++11 lambda.
    @include snippets/create_2d_range.cc
    @include snippets/create_2d_range_task.cc

    @par Example 3 Create a 3D range task using a C++11 lambda.
    @include snippets/create_3d_range.cc
    @include snippets/create_3d_range_task.cc

    @sa mare::launch(group_ptr const&, Body&&)
*/
template<size_t DIMS, typename Body>
inline task_ptr create_ndrange_task(const range<DIMS>& r,
                                    Body&& body)
{
  return ndrange_task_creator<DIMS, Body,
           typename std::is_base_of<body_with_attrs_base_gpu, Body>::type>::
           create(r, std::forward<Body>(body));
}

/** @} */ /* end_addtogroup tasks_creation */

template<typename Body>
inline task_ptr create_ndrange_task(size_t first, size_t last, Body&& body)
{
  //create a 1d range
  mare::range<1> r(first, last);
  return create_ndrange_task(r, std::forward<Body>(body));
}


} // ns mare

