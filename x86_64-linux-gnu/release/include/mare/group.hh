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
/** @file group.hh */
#pragma once

#include <mare/internal/group.hh>

namespace mare {

// ---------------------------------------------------------------
//  USER API
// ---------------------------------------------------------------

/** @addtogroup groups_doc
@{ */
/**
    Returns the name of the group.

    @param group Group.

    @return
    std::string containing the name of the group.

    @throws api_exception If group pointer is NULL.
*/
inline std::string const& group_name(group_ptr const& group) {
  internal::group* ptr = internal::c_ptr(group);
  MARE_API_ASSERT(ptr, "null group_ptr");
  return ptr->get_name();
}
/** @} */ /* end_addtogroup groups_doc */

/** @addtogroup groups_creation
@{ */
/**
    Creates a named group and returns a group_ptr that points to the group.

    Groups can be named to facilitate debugging. Two or more groups can
    share the same name.

    @param name Group name.

    @return
    group_ptr -- Points to the new group.

    @par Example
    @includelineno groups2.cc

    @sa mare::create_group()
*/

inline group_ptr create_group(std::string const& name)
{
  auto g = internal::group_factory::create_leaf_group(name);
  // We use NO_INITIAL_REF because we create the group with
  // ref_count=1.  This allows us to return a group_ptr without
  // performing any atomic operation.
  return group_ptr(g);
}

/**
    Creates a named (const char*) group and returns a group_ptr
    that points to the group.

    Groups can be named to facilitate debugging. Two or more groups can
    share the same name.

    @param name Group name.

    @return
    group_ptr -- Points to the new group.

    @par Example
    @includelineno groups2.cc

    @sa mare::create_group()
*/

inline group_ptr create_group(const char* name)
{
  auto g = internal::group_factory::create_leaf_group(name);
  // We use NO_INITIAL_REF because we create the group with
  // ref_count=1.  This allows us to return a group_ptr without
  // performing any atomic operation.
  return group_ptr(g);
}

/**
    Creates a group and returns a group_ptr that points to the group.

    In the current MARE release, there can be only 32 groups at a
    given time. Trying to create a 33rd group aborts the application.

    @return
    group_ptr -- Points to the new group.

    @par Example
    @includelineno groups1.cc

    @sa mare::create_group(std::string const&)
*/
inline group_ptr create_group()
{
  return create_group("");
}
/**
    Returns a pointer to a group that represents the intersection of
    two groups.

    Some applications require that tasks join more than one group. It
    is possible -- though not recommended for performance
    reasons -- to use mare::launch(group_ptr const&, task_ptr const&)
    or mare::join_group(group_ptr const&, task_ptr const&) repeatedly
    to add a task to several groups. Instead, use
    mare::intersect(group_ptr const&, group_ptr const&) to create a
    new group that represents the intersection of all the groups where
    the tasks need to launch. Again, this method is more performant
    than repeatedly launching the same task into different groups.

    Launching a task into the intersection group also
    simultaneously launches it into all the groups that are part of the
    intersection.

      @note1 Intersection groups do not count towards the 31 maximum
      number of simultaneous groups in the application. Remember that
      group intersection is a somewhat expensive operation. If you
      need to intersect the same groups repeatedly, intersect once,
      and keep the pointer to the group intersection.

    Consecutive calls to <tt>mare::intersect</tt> with the same groups'
    pointer as arguments, return a pointer to the same group. Group
    intersection is a commutative operation.

    You can use the <tt>&</tt> operator instead of
    <tt>mare::intersect</tt>.

    @param a Pointer to the first group.
    @param b Pointer to the second group.

    @return
    group_ptr -- Points to a group that represents the intersection
    of <tt>a</tt> and <tt>b</tt>.

    @par Example
    @includelineno group_intersection1.cc
    @par
    The previous code snippet shows an application with two
    groups, <tt>g1</tt> and <tt>g2</tt>, each with 100 and 200 tasks,
    respectively. In line 16, we intersect <tt>g1</tt> and <tt>g2</tt> into
    <tt>g12</tt>. The returned pointer, <tt>g12</tt>, points to an empty
    group because no task yet belongs to both <tt>g1</tt> and <tt>g2</tt>.
    Therefore, <tt>mare::wait_for(g12)</tt> in line 19 returns
    immediately. The <tt>wait_for</tt> calls in lines 22 and 23
    return only once their tasks complete. In line 30, we launch <tt>t</tt>
    into <tt>g12</tt>. The <tt>wait_for</tt> calls in lines 31 and 32
    return only after <tt>t</tt> completes execution because <tt>t</tt> belongs
    to both <tt>t1</tt> and <tt>t2</tt> (and, of course, <tt>g12</tt>).

    @sa mare::join_group(group_ptr const&,  task_ptr const&)
    @sa mare::launch(group_ptr const&,  task_ptr const&)
*/
inline group_ptr intersect(group_ptr const& a, group_ptr const& b)
{
  using namespace internal;

  group* a_ptr = c_ptr(a);
  group* b_ptr = c_ptr(b);

  // If a group meets nullptr, the result is nullptr.
  if (a_ptr == nullptr || b_ptr == nullptr )
    return group_ptr(nullptr);

  if (a_ptr->is_ancestor_of(b_ptr))
    return b;

  if (b_ptr->is_ancestor_of(a_ptr))
    return a;

  group_ptr retval(internal::lattice::create_meet_node(a_ptr, b_ptr));
  return retval;
}

/** @} */ /* end_addtogroup groups_creation */

/** @addtogroup grouping
@{ */
/**
    Adds a task to group without launching it.

    The recommended way to add a task to a group is by using
    mare::launch(). mare::join_group(group_ptr const& group, task_ptr
    const& task) allows you to add a task into a group without having
    to launch it. Use mare::join_group(group_ptr const& group,
    task_ptr const& task) only when you absolutely need the task
    to belong to a group, but you are not yet ready to launch the task.
    For example, perhaps you want to prevent the group from being empty, so
    you can wait on it somewhere else. This method is slow, and should
    be employed infrequently. It is possible -- though not recommended
    for performance reasons -- to use
    mare::join_group(group_ptr const&, task_ptr const&) repeatedly to
    add a task to several groups. Repeatedly adding a task to the
    same group is not an error.

    Regardless of the method you use to add tasks to a group, the
    following rules always apply:

    - Tasks stay in the group until they complete execution. Once a
      task is added to a group, there is no way to remove it from the
      group.

    - Once a task belonging to multiple groups completes execution,
      MARE removes it from all the groups to which it belongs.

    - Neither completed nor canceled tasks can join groups.

    - Tasks cannot be added to a canceled group.

    @param group Pointer to target group.
    @param task  Pointer to task to add to group.

    @throws api_exception If either <tt>group</tt> or <tt>task</tt> points
    to null

    @par Example
    @includelineno groups_add3.cc

    @sa mare::launch(group_ptr const&,  task_ptr const&)
*/
inline void join_group(group_ptr const& group, task_ptr const& task)
{
  auto g_ptr = internal::c_ptr(group);
  auto t_ptr = internal::c_ptr(task);

  MARE_API_ASSERT(g_ptr, "null group_ptr");
  MARE_API_ASSERT(t_ptr, "null task_ptr");

  t_ptr->join_group(g_ptr, internal::task::utcache_policy::insert);
}

/**
    Adds a task to a group without launching it.

    See join_group(group_ptr const& group, task_ptr const& task).

    @param group Pointer to target group.
    @param task  Pointer to task to add to group.

    @throws api_exception If either <tt>group</tt> or <tt>task</tt> points
    to null.
*/
inline void join_group(group_ptr const& group, unsafe_task_ptr const& task)
{
  auto g_ptr = internal::c_ptr(group);
  auto t_ptr = internal::c_ptr(task);

  MARE_API_ASSERT(g_ptr, "null group_ptr");
  MARE_API_ASSERT(t_ptr, "null task_ptr");

  t_ptr->join_group(g_ptr,internal::task::utcache_policy::insert);
}
/** @} */ /* end_addtogroup grouping */

/** @addtogroup groups_waiting
@{ */
/**
    Waits until all the tasks in it have completed execution or
    have been canceled.

    This function does not return until all tasks in the group
    complete their execution. If new tasks are added to the group
    while wait_for is waiting, this function does not return until all
    those new tasks also complete.  If
    <tt>mare::wait_for(mare::group_ptr)</tt> is called from within a
    task, MARE context switches the task and finds another task to
    run. If called from outside a task, this function blocks the calling
    thread until it returns.

    Waiting for a group intersection means that MARE returns once the
    tasks in the intersection group have completed or executed.

    This method is a safe point. Safe points are MARE API methods
    where the following property holds:
    @par
    The thread on which the task executes before the API call might
    not be the same as the thread on which the task executes after the
    API call.

    @param group Pointer to group.

    @throws api_exception If group points to null.

    @par Example 1
    @includelineno wait_for_group1.cc

    @par Example 2
    @includelineno wait_for_group2.cc

    @sa wait_for(task_ptr const& task)
*/
inline void wait_for(group_ptr const& group)
{
  auto g_ptr = internal::c_ptr(group);
  MARE_API_ASSERT(g_ptr, "null group_ptr");
  g_ptr->wait();
}

inline void
spin_wait_for(group_ptr const& group)
{
  auto g_ptr = internal::c_ptr(group);
  MARE_API_ASSERT(g_ptr, "null group_ptr");

  if (internal::current_task()) {
    g_ptr->wait(); // not spinning inside of a task to avoid wasting cycles
    return;
  }

  size_t spin_count = 200000;
  while (!g_ptr->is_empty(std::memory_order_relaxed)) {
    if (spin_count-- == 0) {
      g_ptr->wait();
      return;
    }
  }
}
/** @} */ /* end_addtogroup groups_waiting */

/** @addtogroup groups_cancel
@{ */
/**
    Cancels all tasks in a group.

    Canceling a group means that:

    - The group tasks that have not started execution will never execute.

    - The group tasks that are executing will be canceled only when they
      call <tt>mare::abort_on_cancel</tt>. If any of these executing tasks
      is a blocking task, MARE executes its cancel handler if
      they had not executed it before.

    - Any tasks added to the group after the group is canceled are also
      canceled.

    Once a group is canceled, it cannot revert to a non-canceled
    state.  mare::cancel(group_ptr const&) returns immediately. Call
    mare::wait_for(group_ptr const&) afterwards to wait for all the
    running tasks to be completed and for all the non-running tasks
    to be canceled.

    @param group Group to cancel.

    @par Example 1
    @includelineno snippets/cancel_group1.cc

    In the example above, we launch 10000 tasks and then sleep for
    some time, so that a few of those 10000 tasks are done, a few
    others are executing, and the majority is waiting to be
    executed. In line 24, we cancel the group, which means that next
    time the running tasks execute <tt>mare::abort_on_cancel()</tt>,
    they will see that their group has been canceled and will
    abort. <tt>wait_for(g)</tt> will not return before the running
    tasks end their execution -- either because they call
    <tt>mare::abort_on_cancel()</tt>, or because they finish writing
    all the messages.

    @par Example 2
    @includelineno examples/cancel-group.cc

    @par Output
    @code
    wait_for returned after 87 tasks executed.
    @endcode

    @sa mare::cancel(task_ptr const&)
    @sa mare::cancel(unsafe_task_ptr const&task)
*/
inline void cancel(group_ptr const& group)
{
  auto g_ptr = internal::c_ptr(group);
  MARE_API_ASSERT(g_ptr != nullptr, "null group_ptr");
  g_ptr->cancel();
  internal::cancel_unlaunched_tasks(g_ptr);
  internal::cancel_blocking_tasks(group);
}

/**
    Checks whether the group is canceled.

    Returns true if the group has been canceled; otherwise,
    returns false.

    @param group  Group pointer.

    @return
    TRUE -- The group is canceled. \n
    FALSE -- The group is not canceled.

    @throws api_exception If group pointer is NULL.

    @sa mare::cancel(group_ptr const& group)
*/
inline bool canceled(group_ptr const& group)
{
  auto g_ptr = internal::c_ptr(group);
  MARE_API_ASSERT(g_ptr != nullptr, "null group_ptr");
  return g_ptr->is_canceled();
}

/** @} */ /* end_addtogroup groups_cancel */

}//namespace mare

/** @cond */
inline
mare::group_ptr operator&(mare::group_ptr const& a, mare::group_ptr const& b)
{
  return mare::intersect(a,b);
}
/** @endcond */

