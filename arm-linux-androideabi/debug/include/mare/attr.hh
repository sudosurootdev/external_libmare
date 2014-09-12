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
/** @file attr.hh */
#pragma once

#include <mare/attrobjs.hh>

#include <mare/internal/attr.hh>

namespace mare{

namespace attr {

/** @addtogroup attributes
@{ */

/** The attr namespace contains the attributes that can be applied to tasks.
    A blocking task is defined as a task that is dependent on
    external (non-MARE) synchronization to make guaranteed forward
    progress. Typically, this includes the completion of I/O
    requests and other OS syscalls with indefinite runtime, but also
    busy-waiting.  It does not include waiting on MARE tasks or groups
    using <tt>mare::wait_for</tt>.

    There are two problems with blocking tasks. First,
    once tasks execute, they take over a thread in MARE's thread
    pool, thus preventing other tasks from executing in the same
    thread.  Because a blocking task spends most of its time
    blocking on an event, we are essentially wasting one of the threads
    in the thread pool. When the programmer decorates a task with the
    <tt>mare::attr::blocking</tt> attribute, MARE ensures that the
    thread pool does not lose thread on the task.

    The second problem involves cancelation. If a blocking task
    is canceled while it is waiting on an external event, we need to
    wake the task up, so that it can execute
    <tt>mare::abort_on_cancel</tt>. This is why blocking
    tasks require an extra lambda function, called the "cancel
    handler".  The cancel handler of a task is executed only once, and
    only if the task is running.

    @par Example 1
    @includelineno blocking_task.cc

    In the example above, we create a blocking task using two lambdas,
    <tt>body</tt> and <tt>cancel_handler</tt>. After launching
    <tt>t</tt> and sleeping for a couple of seconds, we cancel it
    (line 24).  Most likely, by the time we cancel <tt>t</tt>, it will
    be waiting on the condition variable. The cancel handler function
    calls wakes up the task body so that it can abort (line 10)

    @par Example 2
    @includelineno examples/attrblocking.cc
*/
static const internal::task_attr_blocking blocking;

static const internal::task_attr_yield yield;

static const internal::task_attr_opencl gpu;

/** @} */ /* end_addtogroup attributes */

} // namespace attr

#ifdef HAVE_OPENCL
namespace device_attr
{

static const internal::device_attr_available is_available;
static const internal::device_attr_compiler_available compiler_available;
static const internal::device_attr_double_fp_config double_fp_config;
static const internal::device_attr_endian_little little_endian;
static const internal::device_attr_ecc_support ecc_support;
static const internal::device_attr_global_memsize global_mem_size;
static const internal::device_attr_local_memsize local_mem_size;
static const internal::device_attr_max_clock_freq max_clock_freq;
static const internal::device_attr_max_compute_units max_compute_units;
static const internal::device_attr_max_workgroup_size max_work_group_size;
static const internal::device_attr_max_work_item_dims max_work_item_dims;
static const internal::device_attr_max_work_item_sizes max_work_item_sizes;
static const internal::device_attr_device_name name;
static const internal::device_attr_device_vendor vendor;
static const internal::device_attr_device_type_gpu gpu;

}

namespace buffer_attr
{

static const internal::buffer_attr_read_only read_only;
static const internal::buffer_attr_write_only write_only;
static const internal::buffer_attr_read_write read_write;

}
#endif // HAVE_OPENCL

/** @cond */
// Forward declarations
class task_attrs;

MARE_CONSTEXPR task_attrs create_task_attrs();

template<typename Attribute, typename ...Attributes>
constexpr task_attrs create_task_attrs(Attribute const& attr1,
                                       Attributes const&...attrn);

template<typename Attribute>
constexpr bool has_attr(task_attrs const& attrs, Attribute const& attr);

template<typename Attribute>
constexpr task_attrs remove_attr(task_attrs const& attrs,
                                 Attribute const& attr);

template<typename Attribute>
const task_attrs add_attr(task_attrs const& attrs, Attribute const& attr);
/** @endcond */

/** @addtogroup attributes
@{ */
/**
    Checks whether a task_attrs object includes a certain attribute.

    @param attrs task_attrs object to query.
    @param attr Attribute.

    @return
    TRUE -- task_attrs Includes attribute.\n
    FALSE -- task_attrs Does not include attribute.

    @par Example
    @includelineno has_attr1.cc
*/
template<typename Attribute>
constexpr bool has_attr(task_attrs const& attrs, Attribute const& attr) {
  static_assert(std::is_base_of<internal::task_attr_base, Attribute>::value,
                "Invalid task attribute.");
  return ((attrs._mask & attr.value) != 0);
}

/**
    Removes attribute from a task_attr object.

    Returns a new task_attr object that includes all the attributes
    in the original task_attr, except the one specified in the argument list.
    If the attribute was not in the original task_attr object, the returned
    object is identical to the original object.

    @param attrs Original task_attrs object.
    @param attr Attribute.

    @return
    task_attrs -- Includes all the attributes in attrs except attr. If
      task_attrs does not include attr, the returned task_attrs is identical
      to attrs.
*/
template<typename Attribute>
constexpr task_attrs
remove_attr(task_attrs const& attrs, Attribute const& attr)
{
  static_assert(std::is_base_of<internal::task_attr_base, Attribute>::value,
                "Invalid task attribute.");
  return task_attrs(attrs._mask & ~attr.value);
}

/**
    Adds attribute to a mare::task_attrs object.

    Returns a new task_attrs object that includes all the attributes in
    the original task_attr, plus the one specified in the argument list.
    If the attribute was already in the original task_attrs object, the
    returned object is identical to the original object.

    @param attrs Original task_attrs object.
    @param attr Attribute.

    @return
    task_attrs -- Includes all the attibutes in attrs plus attr. If task_attrs
     already includes attr, the returned task_attrs is identical to attrs.

    @par Example
    @includelineno snippets/add_attr1.cc
*/
template<typename Attribute>
const task_attrs add_attr(task_attrs const& attrs, Attribute const& attr) {
  static_assert(std::is_base_of<internal::task_attr_base, Attribute>::value,
                "Invalid task attribute.");
  MARE_API_ASSERT((Attribute::conflicts_with & attrs._mask) == 0,
                  "Conflicting task attributes");
  return task_attrs(attrs._mask | attr.value);
}

/**
    Creates an empty task_attrs object.

    @return
    Empty task_attrs object.\n
    @par Example
    @includelineno create_task_attrs2.cc

    @sa mare::create_task_attrs(Attribute const&, Attributes const&...)
*/
inline MARE_CONSTEXPR task_attrs create_task_attrs() {
  return task_attrs(0);
}

#ifdef ONLY_FOR_DOXYGEN
#error The compiler should not see these methods

/**
    Creates task_attrs object.

    Creates a mare::task_attrs object that summarizes all the
    attributes passed as parameters.

    @param attr1 First attribute.
    @param attrn Last attribute.

    @return
    task_attrs object -- Includes all the attributes passed as parameters.

    @par Example
    @includelineno create_task_attrs3.cc

    @sa mare::create_task_attrs()
*/
template<typename Attribute, typename ...Attributes>
inline constexpr task_attrs create_task_attrs(Attribute const& attr1,
                                              Attributes const&...attrn);

#else

template<typename Attribute, typename ...Attributes>
inline constexpr task_attrs create_task_attrs(Attribute const&,
                                              Attributes const&...) {
  static_assert(internal::
                find_task_attr_conflict<Attribute,
                Attributes...>::value == false,
                "Conflicting task attributes");
  return task_attrs(internal::build_attr_mask<Attribute,
                                              Attributes...>::value);
}

#endif
/** @} */ /* end_addtogroup attributes */

} //namespace mare
