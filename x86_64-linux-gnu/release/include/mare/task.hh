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
/** @file task.hh */
#pragma once

#include <mare/device.hh>

#include <mare/internal/group.hh>
#include <mare/internal/task.hh>
#include <mare/internal/taskfactory.hh>

namespace mare
{

// -----------------------------------------------------------------
// USER API
// -----------------------------------------------------------------
/** @addtogroup sync
@{ */
/**
    Waits for task to complete execution.

    This method does not return until the task completes its
    execution.  It returns immediately if the task has already
    finished.  If <tt>wait_for(mare::task_ptr)</tt> is called from
    within a task, MARE context-switches the task and finds another
    one to run. If called from outside a task (i.e., the main thread),
    MARE blocks the thread until <tt>wait_for(mare::task_ptr)</tt>
    returns.

    This method is a safe point. Safe points are MARE API methods
    where the following property holds:
    @par
    The thread on which the task executes before the API call might not
    be the same as the thread on which the task executes after the API call.

    @param task Pointer to target task.

    @throws api_exception If task points to null.

    @par Example
    @includelineno tasks10.cc

    @sa mare::wait_for(group_ptr const& group)
*/
inline void wait_for(task_ptr const& task)
{
  auto t_ptr = internal::c_ptr(task);
  MARE_API_ASSERT(t_ptr, "null task_ptr");
  t_ptr->wait();
}

/**
    Waits for task to complete execution.

    See mare::wait_for(task_ptr const&).

    The thread on which the task executes before the API call might not
    be the same as the thread on which the task executes after the API call.

    @param task Pointer to target task.

    @throws api_exception If task points to null.
*/
inline void wait_for(unsafe_task_ptr const& task)
{
  auto t_ptr = internal::c_ptr(task);
  MARE_API_ASSERT(t_ptr, "null unsafe_task_ptr");
  t_ptr->wait();
}
/** @} */ /* end_addtogroup sync */

/** @addtogroup tasks_cancelation
@{ */
/**
    Checks whether a task is canceled.

    Use the function to know whether a task is canceled.  If the task
    was canceled -- via cancelation propagation,
    mare::cancel(group_ptr const&) or mare::cancel(task_ptr const&)
    -- before it started executing, mare::canceled(task_ptr const&)
    returns true.

    If the task was canceled -- via mare::cancel(group_ptr const&)or
    mare::cancel(group_ptr const&) -- while it was executing, then
    mare::canceled(task_ptr const&) returns true only if the task
    is not executing any more and it exited via mare::abort_on_cancel()
    or mare::abort_task().

    Finally, if the task completed successfully, mare::canceled(task_ptr
    const&) always returns false.

    @param task Task pointer.

    @return
    TRUE -- The task has transitioned to a canceled state. \n
    FALSE -- The task has not yet transitioned to a canceled state.

    @throws api_exception If task pointer is NULL.

    @par Example:
    @includelineno canceled1.cc
*/
inline bool canceled(task_ptr const& task)
{
  auto t_ptr = internal::c_ptr(task);
  MARE_API_ASSERT(t_ptr, "null task_ptr");
  return t_ptr->is_canceled();
}

/**
    Checks whether a task is canceled.

    @param task Task pointer.

    @return
    TRUE -- The task has transitioned to a canceled state. \n
    FALSE -- The task has not yet transitioned to a canceled state.

    @throws api_exception If task pointer is NULL.

    @sa mare::canceled(task_ptr const& task)
*/
inline bool canceled(unsafe_task_ptr const& task)
{
  auto t_ptr = internal::c_ptr(task);
  MARE_API_ASSERT(t_ptr, "null unsafe_task_ptr");
  return t_ptr->is_canceled();
}

/**
    Cancels task.

    Use <tt>mare::cancel()</tt> to cancel a task and its
    successors. The effects of <tt>mare::cancel()</tt> depend on the
    task status:

    - If a task is canceled before it launches, it never executes
    -- even if it is launched afterwards. In addition, it propagates
    the cancelation to the task's successors. This is called
    "cancelation propagation".

    - If a task is canceled after it is launched, but before it starts
    executing, it will never execute and it will propagate cancelation
    to its successors.

    - If the task is running when someone else calls
    <tt>mare::cancel()</tt>, it is up to the task to ignore the
    cancelation request and continue its execution, or to honor the
    request via <tt>mare::abort_on_cancel()</tt>, which aborts the
    task's execution and propagates the cancelation to the task's
    successors.

    - Finally, if a task is canceled after it completes its
    execution (successfully or not), it does not change its status and
    it does not propagate cancelation.

    @param task Task to cancel.

    @throws api_exception If task pointer is NULL.

    @par Example 1: Canceling a task before launching it:
    @includelineno cancel_task1.cc

    In the example above, we create two tasks <tt>t1</tt> and
    <tt>t2</tt> and create a dependency between them. Notice that, if
    any of the tasks executes, it will raise an assertion. In line 13,
    we cancel <tt>t1</tt>, which causes <tt>t2</tt> to be canceled as
    well. In line 16, we launch <tt>t2</tt>, but it does not matter as
    it will not execute because it was canceled when <tt>t1</tt>
    propagated its cancelation.

    @par Example 2: Canceling a task after launching it, but before it
    executes.
    @includelineno cancel_task2.cc

    In the example above, we create and chain three tasks <tt>t1</tt>,
    <tt>t2</tt>, and <tt>t3</tt>. In line 18, we launch <tt>t2</tt>,
    but it cannot execute because its predecessor has not executed
    yet. In line 21, we cancel <tt>t2</tt>, which means that it will
    never execute. Because <tt>t3</tt> is <tt>t2</tt>'s successor, it
    is also canceled -- if <tt>t3</tt> had a successor, it would also
    be canceled.

    @par Example 3: Canceling a task while it executes.
    @includelineno abort_on_cancel1.cc

    In the example above, task <tt>t</tt>'s will never finish unless
    it is canceled. <tt>t</tt> is launched in line 12. After
    launching the task, we block for 2 seconds in line 15 to ensure
    that <tt>t</tt> is scheduled and prints its messages. In line 18,
    we ask MARE to cancel the task, which should be running by now.
    <tt>mare::cancel()</tt> returns immediately after it marks the
    task as "pending for cancelation". This means that <tt>t</tt>
    might still be executing after <tt>mare::cancel(t)</tt> returns.
    That is why we call <tt>mare::wait_for(t)</tt> in line
    21, to ensure that we wait for <tt>t</tt> to complete its
    execution.  Remember: a task does not know whether someone has
    requested its cancelation unless it calls
    <tt>mare::abort_on_cancel()</tt> during its execution.

    @par Example 4: Canceling a completed task.
    @includelineno cancel_finished_task1.cc

    In the example above, we launch <tt>t1</tt> and <tt>t2</tt> after
    we set up a dependency between them. On line 25, we cancel
    <tt>t1</tt> after it has completed.  By then, <tt>t1</tt> has
    finished execution (we wait for it in line 21) so
    <tt>cancel(t1)</tt> has no effect. Thus, nobody cancels
    <tt>t2</tt> and <tt>wait_for(t2)</tt> in line 28 never returns.

    @sa mare::after(task_ptr const &, task_ptr const &).
    @sa mare::abort_on_cancel().
    @sa mare::wait_for(task_ptr const&);
*/
inline void cancel(task_ptr const& task)
{
  auto t_ptr = internal::c_ptr(task);
  MARE_API_ASSERT(t_ptr, "null task_ptr");
  t_ptr->cancel();
}

/**
    Cancel tasks

    See mare::cancel(task_ptr const& task).

    @param task Task to cancel.

    @throws api_exception If task pointer is NULL.
*/
inline void cancel(unsafe_task_ptr const& task)
{
  auto t_ptr = internal::c_ptr(task);
  MARE_API_ASSERT(t_ptr, "null unsafe_task_ptr");
  t_ptr->cancel();
}

/**
    Aborts execution of calling task if any of its groups is
    canceled or if someone has canceled it by calling mare::cancel().

    MARE uses cooperative multitasking. Therefore, it cannot abort an
    executing task without help from the task. In MARE, each executing
    task is responsible for periodically checking whether it should
    abort. Thus, tasks call mare::abort_on_cancel() to test whether
    they, or any of the groups to which they belong, have been
    canceled.  If true, mare::abort_on_cancel() does not
    return. Instead, it thows mare::abort_task_exception, which the
    MARE runtime catches. The runtime then transitions the task to a
    canceled state and propagates cancelation to the task's
    successors, if any.

    Because mare::abort_on_cancel() never returns, we recommend that
    you use use RAII to allocate and deallocate the resources used
    inside a task. If using RAII in your code is not an option,
    surround mare::abort_on_cancel() with try -- catch, and call throw
    from within the catch block after the cleanup code.

    @throws abort_task_exception If called from a task that has been
    canceled via mare::cancel() or that belongs to a canceled group.

    @throws api_exception  If called from outside a task.

    @par Example 1
    @includelineno examples/abort-on-cancel.cc

    @par Output
    @code
    Task has executed 1 iterations!
    Task has executed 2 iterations!
    ...
    Task has executed 47 iterations!
    @endcode

    @par Example 2
    @includelineno abort_on_cancel3.cc
*/
inline void abort_on_cancel() {
  auto t = internal::current_task();
  MARE_API_ASSERT(t, "Error: you must call abort_on_cancel "
                  "from within a task.");
  t->abort_on_cancel();
}

/**
    Aborts execution of calling task.

    Use this method from within a running task to immediately abort
    that task and all its successors. Like mare::abort_on_cancel(),
    mare::abort_task() never returns. Instead, it throws
    mare::abort_task_exception, which the MARE runtime catches. The
    runtime then transitions the task to a canceled state and propagates
    propagation to the task's successors, if any.

    @throws abort_task_exception If called from a task has been
       canceled via mare::cancel() or a task that belongs to a canceled group.

    @throws api_exception   If called from outside a task.
    @par Example
    @includelineno abort_task1.cc

    @par Output
    @code
    Hello World !
    Hello World!
    ...
    Hello World!
    @endcode
*/
inline void abort_task() {
  auto t = internal::current_task();
  MARE_API_ASSERT(t, "Error: you must call abort_on_cancel from "
                  "within a task." );
  throw mare::abort_task_exception();
}
/** @} */ /* end_addtogroup tasks_cancelation */

/** @addtogroup dependencies
@{ */
/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    Creates a dependency between two tasks (<tt>pred</tt> and
    <tt>succ</tt>) so that <tt>succ</tt> starts executing only after
    <tt>pred</tt> has completed its execution, regardless of how many
    hardware execution contexts are available in the device.  Use this
    method to create task dependency graphs.

    @note1 The programmer is responsible for ensuring that there are
    no cycles in the task graph.

    If <tt>succ</tt> has already been launched, mare::after() will
    throw an api_exception. This is because it makes little sense to
    add a predecessor to a task that might already be running. On the
    other hand, if <tt>pred</tt> successfully completed execution,
    no dependency is created, and mare::after returns immediately.
    If <tt>pred</tt> was canceled (or if it is canceled in the
    future), <tt>succ</tt> will be canceled as well due to cancelation
    propagation.

    @param pred Predecessor task.
    @param succ Successor task.

    @throws api_exception   If <tt>pred</tt> or <tt>succ</tt> are NULL.
    @throws api_exception   If <tt>succ</tt> has already been launched.

    @par Example
    @includelineno examples/after.cc
    Output:
    @code
    Hello World! from task t1
    Hello World! from task t2
    @endcode
    No other output is possible because of the dependency between t1
    and t2.

    @sa mare::before(task_ptr&, task_ptr&).
*/
inline void after(task_ptr const& pred, task_ptr const& succ)
{
  internal::task::add_task_dependence_dispatch(pred, succ);
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::after(task_ptr&, task_ptr&).

    @param pred Predecessor task.
    @param succ Successor task.
*/
inline void after(task_ptr const& pred, unsafe_task_ptr const& succ)
{
  internal::task::add_task_dependence_dispatch(pred, succ);
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::after(task_ptr&, task_ptr&).

    @param pred Predecessor task.
    @param succ Successor task.

*/
inline void after(unsafe_task_ptr const& pred, task_ptr const& succ)
{
  internal::task::add_task_dependence_dispatch(pred, succ);
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.
    @sa mare::after(task_ptr&, task_ptr&).

    @param pred Predecessor task.
    @param succ Successor task.
*/
inline void after(unsafe_task_ptr const& pred, unsafe_task_ptr const& succ)
{
  internal::task::add_task_dependence_dispatch(pred, succ);
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::after(task_ptr&, task_ptr&).

    @param succ Successor task.
    @param pred Predecessor task.

    @par Example
    @code
    // Equivalent to after(task1, task2);
    before(task2, task1);
    @endcode
*/
inline void before(task_ptr& succ, task_ptr& pred)
{
  internal::task::add_task_dependence_dispatch(pred, succ);
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::before(task_ptr&, task_ptr&.

    @param succ Successor task.
    @param pred Predecessor task.
*/
inline void before(task_ptr& succ, unsafe_task_ptr& pred)
{
  internal::task::add_task_dependence_dispatch(pred, succ);
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::before(task_ptr&, task_ptr&.

    @param succ Successor task.
    @param pred Predecessor task.
*/
inline void before(unsafe_task_ptr& succ, task_ptr& pred)
{
  internal::task::add_task_dependence_dispatch(pred, succ);
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::before(task_ptr&, task_ptr&).

    @param succ Successor task.
    @param pred Predecessor task.
*/
inline void before(unsafe_task_ptr& succ, unsafe_task_ptr& pred)
{
  internal::task::add_task_dependence_dispatch(pred, succ);
}
/** @} */ /* end_addtogroup dependencies */

/** @addtogroup tasks_creation
@{ */
/**
    Creates a new task and returns a task_ptr that points to that task.

    This template method creates a task and returns a pointer to it.
    The task executes the <tt>Body</tt> passed as parameter. The
    preferred type of <tt>\<typename Body\></tt> is a C++11 lambda,
    although it is possible to use other types such as function
    objects and function pointers. Regardless of the <tt>Body</tt>
    type, it cannot take any arguments.

    @param body Code that the task will run when the tasks executes.

    @return
    task_ptr -- Task pointer that points to the new task.

    @par Example 1 Create a task using a C++11 lambda (preferred).
    @includelineno snippets/create_task1.cc

    @par Example 2 Create a task using a user-defined class.
    @includelineno snippets/task_class5.cc

    @par Example 3 Create a task using a function pointer.
    @includelineno function_pointer1.cc

    @warning
    Due to limitations in the Visual Studio C++ compiler, this does not
    work on Visual Studio.
    You can get around it by using a lambda function:
    @includelineno function_pointer2.cc

    @sa mare::launch(group_ptr const&, Body&&)
*/
template<typename Body>
inline task_ptr create_task(Body&& body)
{
  typedef internal::body_wrapper<Body> wrapper;
  internal::task* t =  wrapper::create_task(std::forward<Body>(body));

  // We use NO_INITIAL_REF because we create a task with ref_count=1, which
  // allows us to return a task_ptr without doing any atomic operation.

  return mare::task_ptr(t, mare::task_ptr::ref_policy::NO_INITIAL_REF);
}
/** @} */ /* end_addtogroup tasks_creation */

/** @addtogroup execution
@{ */
/**
    Launches task created with mare::create_task(...).

    Use this method to launch a task created with
    mare::create_task(...).  In general, you create tasks using
    mare::create_task(...) if they are part of a DAG. This method
    informs the MARE runtime that the task is ready to execute as soon
    as there is an available hardware context *and* after all its
    predecessors have executed.  Notice that the task is launched into
    a group only if the task already belongs to a group.

    If you do not need to use the task pointer after using
    this method, consider using mare::launch_and_reset()
    instead because it is faster.

    @param task Pointer to task.

    @throws api_exception If task pointer is NULL.

    @par Example
    @includelineno launch1.cc

    @sa mare::launch_and_reset(group_ptr const& group, task_ptr& task)
    @sa mare::create_task(Body&& b)
    @sa mare::launch(unsafe_task_ptr const& task)
    @sa mare::launch(group_ptr const&, task_ptr const&)
*/
inline void launch(task_ptr const& task)
{
  internal::launch_dispatch(task);
}

/**
    Launches task created with mare::create_task(...)

    See mare::launch(task_ptr const& task).

    @param task Pointer to task to launch.

    @throws api_exception If task pointer is NULL.
*/
inline void launch(unsafe_task_ptr const& task)
{
  internal::launch_dispatch(task);
}

#ifdef ONLY_FOR_DOXYGEN

#error The compiler should not see these methods

/**
    Launches task created with mare::create_task(...) into a group.

    Use this method to launch a task created with
    mare::create_task(...). In general, you create tasks using
    mare::create_task(...) if they are part of a DAG. This method
    informs the MARE runtime that the task is ready to execute as soon
    as there is an available hardware context *and* after all its
    predecessors have executed. If you do not need to use the task
    pointer after using this method, consider using
    mare::launch_and_reset() instead because it is faster.

    @param group Pointer to group.
    @param task  Pointer to task.

    @throws api_exception If task pointer or group pointer is NULL.

    @par Example
    @includelineno launch1b.cc

    @sa launch_and_reset(group_ptr const& group, task_ptr& task)
    @sa create_task(Body&& b)
    @sa launch(unsafe_task_ptr const& task)
*/
inline void launch(group_ptr const& group, task_ptr const& task);

/**
    Launches task created with mare::create_task(...) into a group

    See mare::launch(task_ptr const& task).

    @param group Pointer to group.
    @param task Pointer to task.


    @throws api_exception If task pointer or group pointer is NULL.
*/
inline void launch(group_ptr const& group, unsafe_task_ptr const& task);

#endif // ONLY_FOR_DOXYGEN

/**
    Launches task created with mare::create_task(...) into a
    group and resets task pointer

    Use this method to launch into a group a task created with
    mare::create_task(...)  and reset the task pointer in a single
    step.  mare::launch_and_reset() is faster than
    mare::launch(group_ptr const&, task_ptr const&) if <tt>task</tt>
    is the last pointer to the task. This is because MARE runtime can
    assume that it is the sole owner of the task and does not need to
    protect access to it.

    In general, you create tasks using mare::create_task(...) if they
    are part of a DAG. This method informs the MARE runtime that the
    task is ready to execute as soon as there is an available hardware
    context *and* after all its predecessors have executed.

    @param group Pointer to group.
    @param task Pointer to task.

    @throws api_exception If task pointer or group pointer is NULL.

    @par Example
    @includelineno launch_and_reset2.cc

    @sa mare::launch(unsafe_task_ptr const& task)
    @sa mare::launch(group_ptr const&, task_ptr const&)
*/
inline void launch_and_reset(group_ptr const& group, task_ptr& task)
{
  internal::task* t = internal::c_ptr(task);
  MARE_API_ASSERT(internal::c_ptr(task), "null task_ptr");

  // Instead of increasing the task.ref_counter, and then
  // decreasing it, we keep it as-is.
  reset_but_not_unref(task);
  internal::group* g = internal::c_ptr(group);
  t->launch(g, internal::ref_count_policy::do_nothing);
}

/**
    Launches task created with mare::create_task(...) and
    resets task pointer

    Use this method to launch a task created with
    mare::create_task(...)  and reset the task pointer in a single
    step.  mare::launch_and_reset() is faster than
    mare::launch(group_ptr const&, task_ptr const&) if <tt>task</tt>
    is the last pointer to the task. This is because MARE runtime can
    assume that it is the sole owner of the task and does not need to
    protect access to it. Notice that the task is launched into a
    group only if the task already belongs to a group.

    In general, you create tasks using mare::create_task(...) if they
    are part of a DAG. This method informs the MARE runtime that the
    task is ready to execute as soon as there is an available hardware
    context *and* after all its predecessors have executed.

    @param task Pointer to task.

    @throws api_exception If task pointer or group pointer is NULL.

    @par Example
    @includelineno launch_and_reset1.cc

    @sa mare::launch(unsafe_task_ptr const& task)
    @sa mare::launch(task_ptr const&)
*/
inline void launch_and_reset(task_ptr& task)
{
  static group_ptr null_group_ptr;
  launch_and_reset(null_group_ptr, task);
}

/**
    Creates a new task and launches it into a group.

    *This is the fastest way to create and launch a task* into a
    group. We recommend you use it as much as possible. However,
    notice that this method does not return a task_ptr. Therefore, use
    this method if the new task is not part of a DAG. The MARE runtime
    will execute the task as soon as there is an available hardware
    context.

    The new task executes the <tt>Body</tt> passed as parameter.
    The preferred type of <tt>\<typename Body\></tt> is a C++11
    lambda, although it is possible to use other types such as
    function objects and function pointers. Regardless of the
    <tt>Body</tt> type, it cannot take any arguments.

    When launching into many groups, remember that
    group intersection is a somewhat expensive operation. If
    you need to launch into multiple groups several times, intersect
    the groups once and launch the tasks into the intersection.

    @param a_group Pointer to group.
    @param body Task body.

    @throws api_exception If group points to null.

    @par Example 1 Creating and launching tasks into one group.
    @includelineno groups_add1.cc

    @par Example 2 Creating and launching tasks into multiple groups.
    @includelineno group_intersection3.cc

    @sa mare::create_task(Body&& b)
    @sa mare::launch(group_ptr const&, task_ptr const&);
*/
template<typename Body>
inline void launch(group_ptr const& a_group, Body&& body)
{
  internal::launch_dispatch<Body>(a_group, std::forward<Body>(body));
}

/** @} */ /* end_addtogroup execution */

}; //namespace mare

/** @addtogroup dependencies
@{ */
/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::after(task_ptr&, task_ptr&).

    @param pred  Predecessor task.
    @param succ  Successor task.

    @par Example
    @code
    auto t1 = create_task([]{foo();})
    auto t2 = create_task([]{bar();})
    auto t3 = create_task([]{foo();})
    // make t1 execute before t2 before t3
    t1 >> t2 >> t3;
    @endcode
*/
inline mare::task_ptr& operator>>(mare::task_ptr& pred, mare::task_ptr& succ)
{
  mare::internal::task::add_task_dependence_dispatch(pred, succ);
  return succ;
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::after(task_ptr&, task_ptr&).

    @param pred  Predecessor task.
    @param succ  Successor task.
*/
inline mare::unsafe_task_ptr& operator>>(mare::task_ptr& pred,
                                         mare::unsafe_task_ptr& succ)
{
  mare::internal::task::add_task_dependence_dispatch(pred, succ);
  return succ;
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::after(task_ptr&, task_ptr&).

    @param pred  Predecessor task.
    @param succ  Successor task.
*/
inline mare::task_ptr& operator>>(mare::unsafe_task_ptr& pred,
                                  mare::task_ptr& succ)
{
  mare::internal::task::add_task_dependence_dispatch(pred, succ);
  return succ;
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::after(task_ptr&, task_ptr&).

    @param pred  Predecessor task.
    @param succ  Successor task.
*/
inline mare::unsafe_task_ptr& operator>>(mare::unsafe_task_ptr& pred,
                                         mare::unsafe_task_ptr& succ)
{
  mare::internal::task::add_task_dependence_dispatch(pred, succ);
  return succ;
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::before(task_ptr&, task_ptr&).

    @param succ  Successor task.
    @param pred  Predecessor task.

    @par Example
    @code
    auto t1 = create_task([]{foo();})
    auto t2 = create_task([]{bar();})
    auto t3 = create_task([]{foo();})
    // make t1 execute before t2 before t3
    t3 << t2 << t1;
    @endcode
*/
inline mare::task_ptr& operator<<(mare::task_ptr& succ, mare::task_ptr& pred)
{
  mare::internal::task::add_task_dependence_dispatch(pred, succ);
  return pred;
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::before(task_ptr&, task_ptr&).

    @param succ  Successor task.
    @param pred  Predecessor task.
*/
inline mare::unsafe_task_ptr& operator<<(mare::task_ptr& succ,
                                         mare::unsafe_task_ptr& pred)
{
  mare::internal::task::add_task_dependence_dispatch(pred, succ);
  return pred;
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::before(task_ptr&, task_ptr&).

    @param succ  Successor task.
    @param pred  Predecessor task.
*/
inline mare::task_ptr& operator<<(mare::unsafe_task_ptr& succ,
                                  mare::task_ptr& pred)
{
  mare::internal::task::add_task_dependence_dispatch(pred, succ);
  return pred;
}

/**
    Adds the <tt>succ</tt> task to <tt>pred</tt>'s list of successors.

    @sa mare::before(task_ptr&, task_ptr&).

    @param succ  Successor task.
    @param pred  Predecessor task.
*/
inline mare::unsafe_task_ptr& operator<<(mare::unsafe_task_ptr& succ,
                                         mare::unsafe_task_ptr& pred)
{
  mare::internal::task::add_task_dependence_dispatch(pred, succ);
  return pred;
}
/** @} */ /* end_addtogroup dependencies */

namespace mare
{
#ifdef ONLY_FOR_DOXYGEN
#error The compiler should not see these methods

/** @addtogroup tasks_creation
@{ */
/**
    Creates a new task with attributes and returns a task_ptr
    that points to that task.

    This method is identical to mare::create_task(Body&&), except it
    uses a body with attributes, instead of a regular body. The
    preferred type of body is a C++11 lambda, although it is
    possible to use other types such as function objects and function
    pointers. Regardless of its type, the body cannot take any
    arguments.

    @param attrd_body Body with attributes

    @return
    task_ptr -- Task pointer that points to the new task.\n

    @sa mare::create_task(Body&&);
    @sa mare::with_attrs(task_attrs const& attrs, Body &&body);
*/
template<typename Body>
inline task_ptr create_task(body_with_attrs<Body>&& attrd_body);

/**
    Creates a new task with attributes and a cancel handler and
    returns a task_ptr that points to that task.

    This method is identical to mare::create_task(Body&&), except it
    uses a body with attributes and a cancel handler, instead of a
    regular body. The preferred type of the body is a C++11 lambda,
    although it is possible to use other types such as function
    objects and function pointers. Regardless of its type, the body
    cannot take any arguments.

    @param attrd_body Body with attributes and cancel handler

    @return
    task_ptr -- Task pointer that points to the new task.\n

    @par Example
    @includelineno blocking_task.cc

    @sa mare::create_task(Body&&);
    @sa mare::with_attrs(task_attrs const& attrs, Body &&body,
    Cancel_Handler &&handler);
*/
template<typename Body>
inline task_ptr create_task(body_with_attrs<Body,
                            Cancel_Handler>&& attrd_body);

/** @} */ /* end_addtogroup tasks_creation */


/** @addtogroup gpu_tasks_creation
@{ */

/**
    Creates a new task and returns a task_ptr that points to that task.

    This template method creates a task and returns a pointer to it.
    The task executes the <tt>Body</tt> passed as parameter for each
    point in the iteration space of mare::range specified by the first
    argument. The preferred type of <tt>\<typename Body\></tt> is a
    C++11 lambda, although it is possible to use other types such as function
    objects and function pointers. mare::range object passed is used
    as the global size for the OpenCL C kernel.

    @param r Range object (1D, 2D or 3D) representing the iteration space.
    @param attrd_body Body with attributes, kernel object
                      created using <tt>mare::create_kernel</tt>
                      and actual kernel arguments.

    @return
    task_ptr -- Task pointer that points to the new task.

    @par Example 1 Create an ndrange task which runs on the GPU.
    @include snippets/create_buffer_with_host_ptr.cc
    @include snippets/create_task_attrs_gpu.cc
    @include snippets/create_kernel_gpu.cc
    @include snippets/create_1d_range.cc
    @include snippets/create_nd_range_task_gpu.cc

    @sa mare::launch(group_ptr const&, Body&&)
    @sa mare::create_ndrange_task(const range<DIMS>&, Body&&);
    @sa mare::create_task(Body&&)
*/
template<size_t DIMS, typename Body>
inline task_ptr create_ndrange_task(mare::range<DIMS>& r,
                                    body_with_attrs_gpu<
                                      Body,KernelPtr, Kargs...>&& attrd_body);

/** @} */ /* end_addtogroup gpu_tasks_creation */

/** @addtogroup execution
@{ */

/**
    Creates a new task with attributes and launches it into a group.

    *This is the fastest way to create and launch into a group an
    attributed task*.  This method is identical to
    mare::launch(group_ptr const&, Body&&), except that it uses a body
    with attributes, instead of a regular body.

    @param group Pointer to group.
    @param attrd_body Body with attributes

    @throws api_exception If group points to null

    @sa mare::launch(group_ptr const&, Body&&);
*/
template<typename Body>
inline void launch(group_ptr const& group, body_with_attrs<Body>&& attrd_body);

/**
    Creates a new task with attributes and a cancel handler and launches it
    into a group.

    *This is the fastest way to create and launch into a group an
    attributed task with a cancel handler*. This method is identical
    to mare::launch(group_ptr const&, Body&&), except that it uses a
    body with attributes, instead of a regular body.

    Use this method to create blocking tasks.

    @param group Pointer to group.
    @param attrd_body Body with attributes and a cancel handler.

    @throws api_exception If group points to null

    @par Example
    @includelineno blocking_task2.cc

    @sa mare::launch(group_ptr const&, Body&&);
*/
template<typename Body, typename Cancel_Handler>
inline void launch(group_ptr const& group,
                   body_with_attrs<Body,
                   Cancel_Handler>&& attrd_body);

/** @} */ /* end_addtogroup execution */

#endif //ONLY_FOR_DOXYGEN

} // namespace mare

