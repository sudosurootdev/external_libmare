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

#include <atomic>
#include <thread>

#include <mare/internal/log/eventsbase.hh>

// GCC moans about base class having a non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");

namespace mare {
namespace internal {
namespace log {
namespace events {

struct null_event : public base_event<0> { };
struct unknown_event : public base_event<1> { };

// ---------------------------------------------------------------------------
// EVENT LIST. PLEASE KEEP IT ORDERED ALPHABETICALLY.
// ---------------------------------------------------------------------------


/** Somebody tries to cancel a group.*/
struct group_canceled : public single_group_event<__LINE__> {

  /**
      Constructor
      @param g Pointer to group.
      @param success True if it was the first call to g->cancel().
  */
  group_canceled(group* g, bool success) :
    single_group_event<ID>(g),
    _success(success) {}

  /**
     Checks whether this was the first time canceled() was called
     on the group.

     @return
     true -- This was the first time anyone requested the group to be canceled.
     false -- Somebody requested the group to be canceled before.
  */
  bool get_success() const { return _success; }

  /** Event name */
  static const char* get_name() {return "group_canceled";}

private:
  const bool _success;
};

/** New group created */
struct group_created : public single_group_event<__LINE__> {

  /**
     Constructor
     @param g Pointer to group.
  */
  group_created(group* g) :
    single_group_event<ID>(g) {}

  /** Event name */
  static const char* get_name() {return "group_created";}
};

/** Group destroyed (group destructor executes) */
struct group_destroyed : public single_group_event<__LINE__> {

  /**
     Constructor
     @param g Pointer to group.
  */
  group_destroyed(group* g) :
    single_group_event<ID>(g) {}

  /** Event name */
  static const char* get_name() {return "group_destroyed";}
};

/** Group ref count increases */
struct group_reffed : public group_ref_count_event<__LINE__> {

  /** Constructor
      @param g Group whose refcount increased
      @param count New refcount value
  */
  group_reffed(group* g, size_t count) :
    group_ref_count_event<ID>(g, count) {}

  /** Event name */
  static const char* get_name() {return "group_reffed";}
};

/** Group ref count decreases */
struct group_unreffed : public group_ref_count_event<__LINE__> {

  /** Constructor
      @param g Group whose refcount increased
      @param count New refcount value
  */
  group_unreffed(group* g, size_t count) :
    group_ref_count_event<ID>(g, count) {}

  /** Event name */
  static const char* get_name() {return "group_unreffed";}
};

/** Object refcount increased */
struct object_reffed : public object_ref_count_event<__LINE__> {

  /** Constructor
      @param o Object whose refcount increased
      @param count New refcount value
  */
  object_reffed(void* o, size_t count) :
    object_ref_count_event<ID>(o, count) {}

  /** Event name */
  static const char* get_name() {return "object_reffed";}
};

/** Object refcount decreased */
struct object_unreffed : public object_ref_count_event<__LINE__> {

  /** Constructor
      @param g Group whose refcount decreased
      @param count New refcount value
  */
  object_unreffed(void* o, size_t count) :
    object_ref_count_event<ID>(o, count) {}

  /** Event name */
  static const char* get_name() {return "object_unreffed";}
};

/**  User called mare::runtime_shutdown. */
struct runtime_disabled : public base_event<__LINE__> {

  /** Event name */
  static const char* get_name() {return "runtime_disabled";}
};

/**  User calls mare::runtime_init. */
struct runtime_enabled : public base_event<__LINE__> {

  /** Constructor
      @param num_exec_ctx Number of execution contexts.
  */
  runtime_enabled(size_t num_exec_ctx)
  :_num_exec_ctx(num_exec_ctx) {}

  /** Returns number of execution contexts */
  size_t get_num_exec_ctx() const { return _num_exec_ctx; }

  /** Event name */
  static const char* get_name() {return "runtime_enabled";}

private:
  const size_t _num_exec_ctx;
};

/** User creates dependency between two tasks */
struct task_after : public dual_task_event<__LINE__> {

  /** Constructor
      @param pred Pointer to predecessor task
      @param pred Pointer to successor task
  */
  task_after(task* pred, task* succ) :
    dual_task_event<ID>(pred, succ) { }

  /** Returns predecessor task*/
  task* get_pred() const { return get_task(); }

  /** Returns successor task*/
  task* get_succ() const { return get_other_task(); }

  /** Event name */
  static const char* get_name() {return "task_after";}
};

/** Task starts cleanup sequence */
struct task_cleanup : public single_task_event<__LINE__> {

  /** Constructor
      @param t Task pointer
      @param canceled true if task was canceled, false otherwise.
      @param in_utcache true if task was in ut_cache, false otherwise.
  */
  task_cleanup(task *t, bool canceled, bool in_utcache) :
    single_task_event<ID>(t),
    _canceled(canceled),
    _in_utcache(in_utcache) {}

  /** Checks whether the task was canceled or not.

      @return
      true -- The task was canceled.
      false -- The task was not canceled.
  */
  bool get_canceled() const { return _canceled; }

  /** Checks whether the task was in ut_cache or not.

      @return
      true -- The task was canceled.
      false -- The task was not canceled.
  */
  bool get_in_utcache() const { return _in_utcache; }

  /** Event name */
    static const char* get_name() {return "task_cleanup";}

private:
  const bool _canceled;
  const bool _in_utcache;
};

/** MARE creates new task. */
struct task_created : public single_task_event<__LINE__> {

  /**
     Constructor
     @param t Pointer to task.
  */
  task_created(task* g) :
    single_task_event<ID>(g) {}

  /** Event name */
    static const char* get_name() {return "task_created";}
};

/** Task destructor called. */
struct task_destroyed : public single_task_event<__LINE__> {

  /**
     Constructor
     @param t Pointer to task.
  */
  task_destroyed(task* t) :
    single_task_event<ID>(t) {}

  /** Event name */
  static const char* get_name() {return "task_destroyed";}
};

/** Task done. */
struct task_done : public single_task_event<__LINE__> {

  /**
     Constructor
     @param t Pointer to task.
  */
  task_done(task* t) :
    single_task_event<ID>(t) {}

  /** Event name */
  static const char* get_name() {return "task_done";}
};


/** Task starts executing. */
struct task_executes : public single_task_event<__LINE__> {

  /**
     Constructor
     @param t Pointer to task.
  */
  task_executes(task* t) :
    single_task_event<ID>(t) {}

  /** Event name */
  static const char* get_name() {return "task_executes";}
};

/** Task is sent to the runtime */
struct task_sent_to_runtime : public single_task_event<__LINE__> {

  /**
     Constructor
     @param t Pointer to task.
  */
  task_sent_to_runtime(task* t) :
    single_task_event<ID>(t) {}

  /** Event name */
  static const char* get_name() {return "task_sent_to_runtime";}
};

/** Task ref count increases */
struct task_reffed : public task_ref_count_event<__LINE__> {

  /** Constructor
      @param t Task whose refcount increased
      @param count New refcount value
  */
  task_reffed(task* t, size_t count) :
    task_ref_count_event<ID>(t, count) {}

  /** Event name */
  static const char* get_name() {return "task_reffed";}
};

/** Task ref count decreases */
struct task_unreffed : public task_ref_count_event<__LINE__> {

  /** Constructor
      @param t Task whose refcount decreased
      @param count New refcount value
  */
  task_unreffed(task* t, size_t count) :
    task_ref_count_event<ID>(t, count) {}

  /** Event name */
  static const char* get_name() {return "task_unreffed";}
};

/** Someone waits for task to finish */
struct task_wait : public single_task_event<__LINE__> {

  /** Constructor
      @param t Pointer to task to wait for.
      @param wait_required true if we had to wait for the task. False
      otherwise.
  */
  task_wait(task* t, bool wait_required):
    single_task_event<ID>(t),
    _wait_required(wait_required) {
  }

  /** Checks whether we had to wait for the task or not.

      @return
      true -- We had to wait for the task.
      false -- We didn't have to wait for the task.
  */
  bool get_wait_required() const {
    return _wait_required;
  }

  /** Event name */
  static const char* get_name() {return "task_wait";}

private:
  const bool _wait_required;
};

struct ws_tree_new_slab : public base_event<__LINE__> {

  /**
     Constructor
     @param n Pointer to ws_node_base
  */
  ws_tree_new_slab() :
  base_event<ID>() {}

  /** Event name */
  static const char* get_name() {return "ws_tree_new_slab";}
};


struct ws_tree_node_created : public base_event<__LINE__> {

  /**
     Constructor
     @param n Pointer to ws_node_base
  */
  ws_tree_node_created() :
  base_event<ID>() {}

  /** Event name */
  static const char* get_name() {return "ws_tree_node_created";}
};

struct ws_tree_worker_try_own : public base_event<__LINE__> {

  ws_tree_worker_try_own() :
  base_event<ID>() {}

  /** Event name */
  static const char* get_name() {return "ws_tree_worker_try_own";}
};

struct ws_tree_try_own_success : public base_event<__LINE__> {

  ws_tree_try_own_success() :
  base_event<ID>() {}

    /** Event name */
  static const char* get_name() {return "ws_tree_try_own_success";}
};

struct ws_tree_worker_try_steal : public base_event<__LINE__> {

  ws_tree_worker_try_steal() :
  base_event<ID>() {}

  /** Event name */
  static const char* get_name() {return "ws_tree_worker_try_steal";}
};

struct ws_tree_try_steal_success : public base_event<__LINE__> {

  ws_tree_try_steal_success() :
  base_event<ID>() {}

  /** Event name */
  static const char* get_name() {return "ws_tree_try_steal_success";}
};

} //mare::internal::log::events
} //mare::internal::log
} //mare::internal
} //mare

MARE_GCC_IGNORE_END("-Weffc++");
