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
/** @file mareptrs.hh */
#pragma once

#ifdef ONLY_FOR_DOXYGEN

#include<atomic>
#include<utility>

#include<mare/common.hh>

namespace mare  {
/** @addtogroup tasks_doc
    @{ */
/**
    An unsafe pointer to a task.

    This pointer does not do any reference counting and does not affect the
    lifetime of the task. It is safe to use it only as long as a task_ptr
    to the same task also exists. Unsafe task pointers are primarily useful
    because they can be copied and passed around very inexpensively.

    @sa mare::task_ptr
*/
class unsafe_task_ptr {

public:

  /** Default constructor. Constructs null pointer. */
  constexpr unsafe_task_ptr();

  /** Constructs null pointer. */
  constexpr unsafe_task_ptr(std::nullptr_t);

  /**
    Copy constructor. Constructs pointer to the same location as other

    @param other Another pointer used to copy-construct the pointer.
  */
  unsafe_task_ptr(unsafe_task_ptr const& other);

  /**
    Move constructor. Constructs pointer to the same location
    as other using move semantics.

    @param other Another pointer used to copy-construct the pointer.
  */
  unsafe_task_ptr(unsafe_task_ptr&& other);

  /** Destructor. */
  ~unsafe_task_ptr();

  /**
      Assigns address to pointer.

      @param other Pointer used as source for the assignment.

      @return *this
  */
  unsafe_task_ptr& operator=(unsafe_task_ptr const& other);

  /**
      Assigns address to pointer using move semantics.

      @param other Pointer used as source for the assignment.

      @return *this
  */
  unsafe_task_ptr& operator=(unsafe_task_ptr&& other);

  /** Resets pointer. */
  void reset();

  /**
      Swaps the location the pointer points to.

      @param other Pointer used to exchange targets with.
  */
  void swap(unsafe_task_ptr& other);

  /**
      Checks whether there is only one pointer pointing to the object.

      @return
      FALSE -- It always returns false because it is an unsafe pointer.
  */
  bool unique() const;

  /**
      Checks whether the pointer is not a nullptr.

      @return
      TRUE -- The pointer is not a nullptr.\n
      FALSE -- The pointer is a nullptr.
  */
  operator bool() const;

}; //unsafe_task_ptr


/**
    An std::shared_ptr-like managed pointer to a task object.

    mare::create_task returns an object of type mare::task_ptr, which
    is a custom smart pointer to the task object.

    Tasks are reference counted, so they are automatically destroyed when
    no more mare::task_ptr pointers point to them. When a task
    launches, MARE runtime increments the task's reference counter. This
    prevents the task from being destroyed even if all
    mare::task_ptr pointers pointing to the task are reset. The
    MARE runtime decrements the task's reference counter after the task
    completes execution.

    The task reference counter requires atomic operations. Copying a
    mare::task_ptr pointer requires an atomic increment, and an atomic
    decrement when the newly created mare::task_ptr pointer goes out
    of scope. For best results, minimize the number of times your
    application copies group_ptr pointers.

    Some algorithms require constantly passing mare::task_ptr
    pointers. To prevent a decrease in performance, MARE provides
    another task pointer type that does not perform reference
    counting: mare::unsafe_task_ptr.

    The following example demonstrates how to point
    <tt>mare::unsafe_task_ptr</tt> to a task:

    @includelineno unsafe_task_ptr1.cc

    Task lifetime is determined by the the number of mare::task_ptr
    pointers pointing to it. Programmers must avoid using a
    mare::unsafe_task_ptr unless there is at least one
    mare::task_ptr pointing to it because it will likely cause
    memory corruption or segmentation fault.  You can use a
    mare::unsafe_task_ptr pointer in any API method in which
    you can use a mare::task_ptr, with the exception of
    mare::launch_and_reset(mare::task_ptr&).

    @warning Never use a reference to a mare::task_ptr pointer, as it
    can cause undefined behavior. This is because some MARE API
    methods are highly optimized to support the case where there is
    only one mare::task_ptr pointing to a task. Unfortunately,
    MARE cannot always prevent the programmer from taking a reference
    to a mare::task_ptr, so it is up to the programmer to ensure that
    this does not happen.

    @warning DO NOT reference a MARE task pointer like the following example:

    @includelineno reference_error1.cc

    Instead, copy the pointer.

    @includelineno reference_error2.cc

    Or use a mare::unsafe_task_ptr (of course, ensure that <tt>t1</tt>
    does not go out of scope):

    @includelineno reference_error3.cc

    @sa mare::group_ptr
    @sa mare::unsafe_task_ptr
*/
class task_ptr {

public:
  /** Default constructor. Constructs null pointer. */
  constexpr task_ptr();

  /** Constructs null pointer. */
  constexpr task_ptr(std::nullptr_t);

  /**
      Copy constructor. Constructs pointer to the same location as other
      and increases reference count to task.

      @param other Another pointer used to copy-construct the pointer.
  */
  task_ptr(task_ptr const& other);

  /**
      Move constructor. Constructs pointer to the same location
      as other using move semantics. Does not increase the reference count to
      the task.

      @param other Another pointer used to copy-construct the pointer.
  */
  task_ptr(task_ptr&& other);

  /**
      Destructor. It will destroy the target task if it is the last
      task_ptr pointing to to the same task and has not yet been launched.
  */
  ~task_ptr();

  /**
      Assigns address to pointer and increases reference counting to it.

      @param other Pointer used as source for the assignment.

      @return *this
  */
  task_ptr& operator=(task_ptr const& other);

  /**
      Resets the pointer.

      @return *this
  */
  task_ptr& operator=(std::nullptr_t);

  /**
      Assigns address to pointer using move semantics.

      @param other Pointer used as source for the assignment.

      @return *this
  */
  task_ptr& operator=(task_ptr&& other);

  /**
      Swaps the location the pointer points to.

      @param other Pointer used to exchange targets with.
  */
  void swap(task_ptr& other);

  /**
      Returns unsafe_task_ptr pointing to the same task.
      @return
      unsafe_task_ptr -- Unsafe pointer pointing to the same task.
  */
  unsafe_task_ptr get();

  /** Resets the pointer. */
  void reset();

  /**
      Checks whether the pointer is not a nullptr.

      @return
      TRUE -- The pointer is not a nullptr.\n
      FALSE -- The pointer is a nullptr.
  */
  explicit operator bool() const;

  /**
      Counts number of task_ptr pointing to same task.

      This is an expensive operation as it requires an atomic
      operation. Use it sparingly.

      @return
      size_t -- Number of task_ptr pointing to the same task.
  */
  size_t use_count() const;

  /**
      Checks whether there is only one pointer pointing to the task.

      @return
      TRUE -- There is only one pointer pointing to the task.

      FALSE -- There is more than one pointer pointing to the task,
               or the pointer is null.
  */
  bool unique() const;
}; //task_ptr
/** @} */ /* end_addtogroup tasks_doc */

/**
    @addtogroup groups_doc
    @{ */
/**
    An std::shared_ptr-like managed pointer to a group object.

    The method void mare::create_group() returns an object of type
    mare::group_ptr, which is a custom smart pointer to a group
    object. Groups are reference counted, and they are automatically
    destroyed when they have no tasks and there are no more
    mare::group_ptr pointers pointing to them.  This means that even
    if the user has no pointers to the group, MARE will not destroy
    the group until all its tasks complete.

    The group reference counter requires atomic operations. Copying a
    mare::group_ptr pointer requires an atomic increment, and an
    atomic decrement when the newly created mare::group_ptr pointer
    goes out of scope. For best results, minimize the number of times
    your application copies group_ptr pointers.

    The current MARE release does not support mare::unsafe_group_ptr
    yet, but a future one will.

    @warning Never use a reference to a mare::group_ptr pointer, as it
    can cause undefined behavior. This is because some MARE API
    methods are highly optimized to support the case where there is
    only one mare::group_ptr pointing to a group. Unfortunately, MARE
    cannot always prevent the programmer from taking a reference to a
    mare::group_ptr object, so it is up to the programmer to ensure
    that this does not happen.

    @sa mare::task_ptr
*/
class group_ptr {

public:
  /** Default constructor. Constructs null pointer. */
  constexpr group_ptr();

  /** Constructs null pointer. */
  constexpr group_ptr(std::nullptr_t);

  /**
      Copy constructor. Cosntructs pointer to the same location as other
      and increases reference count to group.

      @param other Another pointer used to copy-construct the pointer.
  */
  group_ptr(group_ptr const& other);

  /**
      Move constructor. Constructs pointer to the same location
      as other using move semantics. Does not increase the reference count to
      the group.

      @param other Another pointer used to copy-construct the pointer.
  */
  group_ptr(group_ptr&& other);

  /**
      Destructor. It will destroy the target group if it is the last
      group_ptr pointing to the same group and has not yet been launched.
  */
  ~group_ptr();

  /**
      Assigns address to pointer and increases reference counting to it.

      @param other Pointer used as source for the assignment.

      @return *this
  */
  group_ptr& operator=(group_ptr const& other);

  /**
      Resets the pointer.

      @return *this
  */
  group_ptr& operator=(std::nullptr_t);

  /**
      Assigns address to pointer using move semantics.

      @param other Pointer used as source for the assignment.

      @return *this
  */
  group_ptr& operator=(group_ptr&& other);

  /**
  Swaps the location the pointer points to.

      @param other Pointer used to exchange targets with.
  */
  void swap(group_ptr& other);

  /** Resets the pointer. */
  void reset();

  /**
      Checks whether the pointer is not a nullptr.

      @return
       TRUE -- The pointer is not a nullptr.\n
       FALSE -- The pointer is a nullptr.
  */
  explicit operator bool() const;

  /**
      Counts number of group_ptr pointing to same group.

      This is an expensive operation as it requires an atomic
      operation. Use it sparingly.

      @return
      size_t -- Number of group_ptr pointing to the same group.
  */
  size_t use_count() const;

  /**
      Checks whether there is only one pointer pointing to the group.

      @return
      TRUE -- There is only one pointer pointing to the group.

      FALSE -- There is more than one pointer pointing to the group, or
               the pointer is null.
  */
  bool unique() const;
}; //group_ptr

/** @} */ /* end_addtogroup groups_doc */

/** @cond HIDDEN */
/**
    An std::shared_ptr-like managed pointer to a device object.
    Currently only GPU devices are supported.

    Devices on a platform are enumerated during MARE startup
    and a vector of device_ptr is created.

    Devices are reference counted, so they are automatically destroyed when
    no more mare::device_ptr pointers point to them. The
    MARE runtime decrements the device's reference counter during shutdown.
**/
class device_ptr {

public:
  /**  Default constructor. Constructs null pointer.*/
  constexpr device_ptr();

  /** Constructs null pointer. */
  constexpr device_ptr(std::nullptr_t);

  /**
     Copy constructor. Cosntructs pointer to the same location as other
     and increases reference count to task.

     @param other Another pointer used to copy-construct the pointer.
  */
  device_ptr(device_ptr const& other);

  /**
     Move constructor. Cosntructs pointer to the same location
     as other using move semantics. Does not increase the reference count to
     the device

     @param other Another pointer used to copy-construct the pointer.
  */
  device_ptr(device_ptr&& other);


  /**
      Destructor. It will destroy the target device if it is the last
      device_ptr pointing to to the same task and hasn't been launched yet.
  */
  ~device_ptr();

  /**
      Assigns address to pointer and increases reference counting to it.

      @param other Pointer used as source for the assignment.
      @return *this
  */
  device_ptr& operator=(device_ptr const& other);


  /**
      Resets the pointer.

      @return *this
  */
  device_ptr& operator=(std::nullptr_t);

  /**
      Assigns address to pointer using move semantics

      @param other Pointer used as source for the assignment.
      @return *this
  */
  device_ptr& operator=(device_ptr&& other);

  /**
      Swaps the location the pointer points to

      @param other Pointer used to exchange targets with
  */
  void swap(device_ptr& other);

  /**
      Returns unsafe_device_ptr pointing to the same device
      @return
      unsafe_device_ptr - Unsafe pointer pointing to the same device.
  */
  unsafe_device_ptr get();

  /** Resets the pointer. */
  void reset();

  /**
      Checks whether the pointer is not a nullptr.

      @return
      TRUE - The pointer is not a nullptr.\n
      FALSE - The pointer is a nullptr
  */
  explicit operator bool() const;

  /**
      Counts number of device_ptr pointing to same device.

      This is an expensive operation as it requires an atomic
      operation. Use it sparingly.

      @return
      size_t - Number of device_ptr pointing to the same device.
  */
  size_t use_count() const;

  /**
      Checks whether there is only one pointer pointing to the device.

      @return
      TRUE - There is only one pointer pointing to the device.
      FALSE - There is more than one pointer pointing to the device,
              or the pointer is null.
  */
  bool unique() const;
}; //device_ptr

/** @endcond */

}//namespace mare


/** @addtogroup tasks_doc
@{ */
/**
    Compares two unsafe_task_ptr.

    @param a Pointer.
    @param b Pointer.

    @return
    TRUE -- The two pointers point to the same task.\n
    FALSE -- The two pointers do not point to the same task.

*/
inline bool operator==(mare::unsafe_task_ptr const& a,
                       mare::unsafe_task_ptr const& b);

/**
    Compares an unsafe_task_ptr to nullptr.

    @param a Pointer.

    @return
    TRUE -- a points to null.\n
    FALSE -- a does not point to null.
*/
inline bool operator==(mare::unsafe_task_ptr const& a, std::nullptr_t);

/**
    Compares an unsafe_task_ptr to nullptr.

    @param a Pointer.

    @return
    TRUE -- a points to null.\n
    FALSE -- a does not point to null.
*/
inline bool operator==(std::nullptr_t, mare::unsafe_task_ptr const& a);

/**
    Compares two unsafe_task_ptr.

    @param a Pointer.
    @param b Pointer.

    @return
    TRUE -- The two pointers point to the same task.\n
    FALSE -- The two pointers do not point to the same task.
*/
inline bool operator!=(mare::unsafe_task_ptr const& a,
                       mare::unsafe_task_ptr const& b);

/**
    Compares an unsafe_task_ptr to nullptr.

    @param ptr Pointer.

    @return
    TRUE -- ptr does not point to null.\n
    FALSE -- ptr points to null.
*/
inline bool operator!=(mare::unsafe_task_ptr const& ptr, std::nullptr_t);

/**
    Compares an unsafe_task_ptr to nullptr.

    @param ptr Pointer.

    @return
    TRUE -- ptr does not point to null.\n
    FALSE -- ptr points to null.
*/
inline bool operator!=(std::nullptr_t, mare::unsafe_task_ptr const& ptr);

/**
    Compares two task_ptr.

    @param a Pointer.
    @param b Pointer.

    @return
    TRUE -- The two pointers point to the same task.\n
    FALSE -- The two pointers do not point to the same task.
*/
inline bool operator==(mare::task_ptr const& a, mare::task_ptr const& b);

/**
    Compares a task_ptr to nullptr.

    @param ptr Pointer.

    @return
    TRUE -- ptr points to null.\n
    FALSE -- ptr does not point to null.
*/
inline bool operator==(mare::task_ptr const& ptr, std::nullptr_t);

/**
    Compares a task_ptr to nullptr.

    @param ptr Pointer.

    @return
    TRUE -- ptr points to null.\n
    FALSE -- ptr does not point to null.
*/
inline bool operator==(std::nullptr_t, mare::task_ptr const& ptr);

/**
    Compares two task_ptr.

    @param a Pointer.
    @param b Pointer.

    @return
    TRUE -- The two pointers do not point to the same task.\n
    FALSE -- The two pointers point to the same task.
*/
inline bool operator!=(mare::task_ptr const& a, mare::task_ptr const& b);

/**
    Compares a task_ptr to nullptr.

    @param ptr Pointer.

    @return
    TRUE -- ptr does not point to null.\n
    FALSE - ptr points to null.
*/
inline bool operator!=(mare::task_ptr const& ptr, std::nullptr_t);

/**
    Compares a task_ptr to nullptr.

    @param ptr Pointer.

    @return
    TRUE -- ptr does not point to null.\n
    FALSE -- ptr points to null.
*/
inline bool operator!=(std::nullptr_t, mare::task_ptr const& ptr);

/**
    Compares two group_ptr.

    @param a Pointer.
    @param b Pointer.

    @return
    TRUE -- The two pointers point to the same group.\n
    FALSE -- The two pointers do not point to the same group.
*/
inline bool operator==(mare::group_ptr const& a, mare::group_ptr const& b);

/**
    Compares a group_ptr to nullptr.

    @param ptr Pointer.

    @return
    TRUE -- ptr points to null.\n
    FALSE -- ptr does not point to null
*/
inline bool operator==(mare::group_ptr const& ptr, std::nullptr_t);

/**
    Compares a group_ptr to nullptr

    @param ptr Pointer.

    @return
    TRUE -- ptr points to null.\n
    FALSE -- ptr does not point to null.
*/
inline bool operator==(std::nullptr_t, mare::group_ptr const& ptr);

/**
    Compares two group_ptr.

    @param a Pointer.
    @param b Pointer.

    @return
    TRUE -- The two pointers do not point to the same group.\n
    FALSE -- The two pointers point to the same group.
*/
inline bool operator!=(mare::group_ptr const& a, mare::group_ptr const& b);

/**
    Compares a group_ptr to nullptr.

    @param ptr Pointer.

    @return
    TRUE -- ptr does not point to null.\n
    FALSE -- ptr points to null.
*/
inline bool operator!=(mare::group_ptr const& ptr, std::nullptr_t);

/**
    Compares a group_ptr to nullptr.

    @param ptr Pointer.

    @return
    TRUE -- ptr does not point to null.\n
    FALSE -- ptr points to null.
*/
inline bool operator!=(std::nullptr_t, mare::group_ptr const& ptr);

/** @} */ /* end_addtogroup tasks_doc */

#endif //ONLY_FOR_DOXYGEN
