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
#include <cstdint>
#include <mutex>
#include <utility>

#include <mare/attr.hh>
#include <mare/exceptions.hh>
#include <mare/internal/compat.h>
#include <mare/internal/debug.hh>
#include <mare/internal/log/log.hh>
#include <mare/internal/macros.hh>
#include <mare/internal/mareptrs.hh>
#include <mare/internal/runtime.hh>
#include <mare/internal/taskstorage.hh>
#include <mare/internal/taskstate.hh>
#include <mare/internal/templatemagic.hh>
#include <mare/internal/tls.hh>
#include <mare/internal/ull.hh>
#include <mare/internal/utcache.hh>
#include <mare/runtime.hh>

namespace mare { namespace internal {

class scheduler;


// forwarding declaration
namespace test {
class task_tests;
} //namespace test

inline task* current_task() {
  auto ti = tls::get();
  return ti->current_task();
}

inline task* set_current_task(task* t)
{
  auto ti = tls::get();
  return ti->set_current_task(t);
}

/**
     A Task is a piece of code together with a context and dependences.
*/

class task : public ref_counted_object<task> {

private:

  std::atomic<task_state::type> _state;
  std::atomic<group*> _group;
  task_attrs _attrs;
  ull<task, 2,16> _successors;
  storage_map _taskls_map;

  mutable std::mutex _mutex;
  bool _runtime_owned;
  std::atomic<scheduler*> _scheduler;

  const log::task_id _log_id;

public:
  enum utcache_policy {
    insert,
    remove,
    do_nothing
  };

  virtual void execute() = 0;
  virtual void cancel_notify() = 0;

  /// Returs log id for task object
  ///
  /// @return log::task_id
  const log::task_id get_log_id() const {
    return _log_id;
  }

  /// Returns source for task object
  ///
  /// @return uintptr_t
  virtual uintptr_t get_source() const { return 0; }

  /// Returns task attributes
  task_attrs get_attrs() const {
    return _attrs;
  }

  /// @brief Checks whether a task is anonymous.
  ///
  /// @return
  /// true - The task is anonymous.
  /// false - The task is not anonymous.
  bool is_anonymous() const {
    return has_attr(_attrs, internal::attr::anonymous);
  }

  /// @brief Checks whether a task is blocking.
  ///
  /// @return
  /// true - The task is blocking.
  /// false - The task is not blocking.
  bool is_blocking() const { return has_attr(_attrs, mare::attr::blocking); }

  /// @brief Checks whether a task is long running.
  ///
  /// @return
  /// true - The task is long running.
  /// false - The task is not long running.
  bool is_long_running() const {
    return has_attr(_attrs, mare::internal::attr::long_running);
  }

  /// @brief Checks whether a task is a stub task.
  ///
  /// @return
  /// true - The task is a stub task.
  /// false - The task is not a stub task.
  bool is_stub() const { return has_attr(_attrs, internal::attr::stub); }


  /// @brief Checks whether a task is a pfor task.
  ///
  /// @return
  /// true - The task is a pfor task
  /// false - The task is not a pfor task.
  bool is_pfor() const { return has_attr(_attrs, internal::attr::pfor); }

  /// @brief Checks whether a task is cancelable.
  ///
  /// @return
  /// true - The task is cancelable.
  /// false - The task is not cancelable.
  bool is_cancelable() const {
    return !has_attr(_attrs, internal::attr::non_cancelable);
  }

  /// @brief Returns true if the task is a yield task.
  ///
  /// @return
  /// true or false
  bool is_yield() const { return has_attr(_attrs, internal::attr::yield); }

  bool is_gpu() const { return has_attr(_attrs, mare::attr::gpu); }

  /// @brief Returns the task state.
  ///
  /// @return
  /// task_state object.
  task_state get_state(std::memory_order mem_order =
                       std::memory_order_seq_cst) const {
    return _state.load(mem_order);
  }

  /// @brief Chooses which add_task_dependence to call
  ///
  /// @param pred Predecessor task
  /// @param succ Successor task
  template<typename PredTask, typename SuccTask>
  static void add_task_dependence_dispatch(PredTask& pred, SuccTask& succ);

  /// @brief Adds a dependence between pred and succ.
  ///
  /// @param pred task to execute first.
  /// @param succ task to execute second.
  static void add_task_dependence(task* pred, task* succ);

  /// Several schedulers might compete for the same task, but only one
  /// of them should be allowed to own it. Schedulers must call
  /// request_ownership before executing the task. request_ownership
  /// will return true only once.
  ///
  /// NOTE: this was designed for hybrid tasks (e.g., CPU, GPU, DSP),
  /// where tasks are enqueued for each device.
  ///
  /// @return
  /// true - The caller owns the task.\n
  /// false - Some other caller owns the task.
  bool request_ownership(scheduler& sched) {
    auto prev = std::atomic_exchange_explicit(&_scheduler,
                                              &sched,
                                              std::memory_order_seq_cst);
    MARE_INTERNAL_ASSERT(!prev,
                         "Ownership request currently should always succeed,"
                         " previous: %p, requested: %p",
                         prev, &sched);
    return !prev;
  }

  /// @brief returns scheduler of current task, or nullptr if none
  /// @return task's scheduler or nullptr
  scheduler* get_scheduler() {
    return _scheduler.load(std::memory_order_seq_cst);
  }

public:

  /// Returns the scheduler that owns the task.
  ///
  /// @return
  /// scheduler* - Pointer to the scheduler that owns the task.
  /// nullptr - The task is not owned by any scheduler.
  static scheduler* current_scheduler() {
    if (auto t = current_task())
      return t->get_scheduler();
    return nullptr;
  }

  /// @brief Marks task as launched and, if possible,
  /// sends it to the runtime for execution.
  ///
  /// This is equivalent to launch(nullptr, ref_count_policy::increase);
  ///
  /// @return
  /// true - The task was sent to the runtime for execution.\n
  /// false - The task was not set to the runtime for execution.
  bool launch() {
    return launch(nullptr, ref_count_policy::increase);
  }

  /// @brief Marks task as launched and, if possible,
  /// sends it to the runtime for execution.
  ///
  /// This method is equivalent to launch(group,
  /// ref_count_policy::increase);
  ///
  /// @param g Target group
  ///
  /// @return
  /// true - The task was sent to the runtime for execution.\n
  /// false - The task was not set to the runtime for execution.
  bool launch(group* g) {
    return launch(g, ref_count_policy::increase);
  }

  /// @brief Marks task as launched and, if possible,
  /// sends it to the runtime for execution.
  ///
  /// The caller can specify the policy for the reference counting.
  /// In some situations, launching a task does not require that its
  /// reference count gets increased. For example, this happens when
  /// the uses launch(group_ptr, lambda expression) or when the user
  /// calls launch_and_reset.
  ///
  /// @param g Target group
  /// @param policy Reference count policy.
  ///
  ///
  /// @return
  /// true - The task was sent to the runtime for execution.\n
  /// false - The task was not set to the runtime for execution.
  bool launch(group* g, ref_count_policy policy);

  /// Suspends the caller until this task has finished execution.
  void wait();

  /// Marks the task as not being in the unlaunched task cache
  ///
  /// @param mem_order Memory synchronization ordering for the
  /// operation. The default is std::memory_order_seq_cst.
  ///
  void reset_state_in_utcache(std::memory_order mem_order =
                                     std::memory_order_seq_cst) {
    _state.fetch_and(task_state::IN_UTCACHE_MASK, mem_order);
  }

  /// Yields from task back to scheduler.
  void yield();

  /// Cancel task and its children without grabbing the task lock
  void cancel_unlocked() {
    //Some tasks are not cancelable, i.e. stub_tasks.
    if (!is_cancelable())
      return;

    cancel_impl();
  }

  /// Cancel task and its children grabbing the task lock
  void cancel() {
    //Some tasks are not cancelable, i.e. stub_tasks.
    if (!is_cancelable())
      return;

    std::lock_guard<std::mutex> lock(_mutex);
    cancel_impl();
  }

  /// Returns true if  the user has requested the task to be canceled
  /// @todo Check that the group has been canceled as well
  ///
  /// @return
  /// true - The user has canceled the task or one of the groups
  ///         it belongs to.
  /// false - The user has not canceled the task nor any of the groups
  ///         it belongs to
  bool is_canceled() const;

  /// Throws an exception that will be caught by MARE's runtime.
  /// The programmer calls this method within a task to acknowledge
  /// that the task is canceled.
  void abort_on_cancel();

  /// Checks whether the runtime has the only pointer to the task.
  ///
  /// @returns
  /// true - The runtime owns the only ponter to the task.
  /// false - The user or another task may have a pointer to the task.
  bool is_runtime_owned() const {
    return _runtime_owned;
  }

  /// Indicates that the runtime is the only one with a valid
  /// pointer to the task.
  void set_runtime_owned() {
    MARE_INTERNAL_ASSERT(is_runtime_owned() == false,
                         "The runtime already owns task %p.",
                         this);
    _runtime_owned = true;
  }

  /// Destructor. The user is not expected to delete a task_ptr because
  /// it is a smart pointer.
  virtual ~task() {
    log::log_event(log::events::task_destroyed(this));
  }

  /// @brief Adds task to group g.
  ///
  /// Adds task to group g using locking. Because it is slower,
  /// we expect this method to be called only when we can't use
  /// its sister method, join_group_fast
  ///
  ///
  /// @param g Target group
  /// @param join_policy Indicates whether the task should be added to
  ///        the unlaunched task cache.
  void join_group(group* g, utcache_policy join_policy);

  /// @brief Returns the group the task belongs to.
  ///
  /// @param mem_order Memory synchronization ordering for the
  /// operation. The default is std::memory_order_seq_cst.
  ///
  /// @return Pointer to the group the task belongs to.
  group* get_group(std::memory_order mem_order =
                   std::memory_order_seq_cst) const {
    return _group.load(mem_order);
  }

  /// @brief Prepares the task to be inserted in the ut_cache.
  ///
  /// @param g Target group
  /// @param mem_order Memory synchronization ordering for the
  /// operation. The default is std::memory_order_seq_cst.
  void prepare_for_ut_cache(group* g, std::memory_order mem_order =
                            std::memory_order_seq_cst);

  /// @brief Returs a string that describes the status of the task.
  ///
  /// @return String that describes the status of the task.
  std::string to_string();

  /// Changes the task status, traverses successor vector to reduce
  /// their predecessor counts and removes the task from the groups it
  /// belongs to.
  ///
  /// If this method is changed, cleanup_trigger_task() needs to be
  /// changed accordingly.
  virtual void schedule();

  /// Cleans up trigger task, marking the task as completed and
  /// traverse its successors, similar to schedule().  Note that
  /// the task function is not actually executed.
  ///
  /// If this method is changed, schedule() needs to be changed
  /// accordingly.
  void cleanup_trigger_task();


  void execute_inline(group*);

  /// Called after task completed execution or if it's cancelled,
  /// it takes care of clean up.
  ///
  /// @param must_cancel
  /// true - the task must transition to CANCELED state.
  /// false - the task must transition to COMPLETED state
  /// @param in_utcache
  /// true - the task is in utcache.
  /// false - the task is not in utcache.
  void task_completed(bool must_cancel, bool in_utcache);

  /// This functor is called by the shared_ptr object when the
  /// reference counter for the pointer gets to 0. If operator()
  /// returns true, then the shared_ptr can destroy the object
  /// pointed by the shared_ptr. If it returns false, it doesn't.
  ///
  /// The reason for this struct is that we can't delete unlaunched
  /// tasks with predecessors, because tasks dependences do not count
  /// towards the number of references.
  struct should_delete_functor {
    bool operator()(task* t) {

      MARE_API_DETAILED_ASSERT(t, "null task pointer");
      MARE_API_DETAILED_ASSERT(t->use_count() == 0,
                               "Deleter called before counter hit 0.");
      MARE_API_DETAILED_ASSERT(!t->get_state().is_running(),
                               "Deleting running task.");
      MARE_API_DETAILED_ASSERT(!t->get_state().is_ready(),
                               "Deleting ready task.");
      MARE_API_DETAILED_ASSERT(!(t->get_state().is_launched() &&
                                 !t->get_state().is_done()),
                               "Deleting launched task.");

      // If the task has already been ran, there is nothing else for
      // us to do here we can return true and delete the task. We
      // first try with a relaxed load. We hope that this load catches
      // most of the cases.
      if (t->get_state(std::memory_order_relaxed).is_done())
        return true;

      return should_delete_detailed_check(t);
    }

    bool should_delete_detailed_check(task* t);
  }; // should_delete_functor

  /// Returns reference to task local storage
  storage_map& taskls() {
    return _taskls_map;
  }


protected:

  /// Constructor.
  ///
  /// @param g Target group.
  /// @param a Task attributes.
  task(group* g, task_attrs a):
    ref_counted_object(1),
    _state(get_initial_state(a)),
    _group(g),
    _attrs(a),
    _successors(),
    _taskls_map(),
    _mutex(),
    _runtime_owned(false),
    _scheduler(nullptr),
    _log_id()
  {
    MARE_API_ASSERT(runtime_available(),
                    "Cannot create task unless MARE runtime is initialized");
    log::log_event(log::events::task_created(this));
  }

  // Disable all copying and movement, since other objects may have
  // pointers to the internal fields of this object.
  MARE_DELETE_METHOD(task(task const&));
  MARE_DELETE_METHOD(task(task&&));
  MARE_DELETE_METHOD(task& operator=(task const&));
  MARE_DELETE_METHOD(task& operator=(task&&));

private:

  /// @Decreases the predecessor counter of the task by 1.
  ///
  /// When the
  /// predecessor counter reaches 0:
  ///
  /// - If the task is ready, send it to the runtime
  /// - if it has no predecessors, but it is marked for cancelation,
  ///   then cancel it.
  void predecessor_completed();

  /// Increases the predecessor counter of the task by 1
  void predecessor_added() {
    task_state prev_state(_state.fetch_add(1, std::memory_order_seq_cst));
    MARE_API_ASSERT(prev_state.get_num_predecessors() != task_state::MAX_PREDS,
                    "Task %p has reached the maximum number of "
                    "predecessors: %s", this,
                    to_string().c_str());
  }

  /// Removes *this from the unlaunched task cache, propagates the
  /// task completion or cancelation to its successors, resets the
  /// successors data structure and removes *this from all its groups.
  ///
  /// Make sure that you hold the task mutex when calling this method.
  ///
  /// @param canceled   Indicates whether the task was canceled.
  /// @param in_utcache Indicates whether the task is in the
  ///                   unlaunched task cache
  void cleanup_unlocked(bool canceled, bool in_utcache);

  /// @brief Propagates cancelation to the successors of *this.
  void propagate_cancelation() {
    static const auto l = [] (task* succ) {
      succ->cancel();
    };
    _successors.for_each(l);
    leave_groups();
  }

  /// Cancels the task.
  void cancel_impl();

  /// Sets the task state to RUNNING if the task has not been
  /// canceled.
  ///
  /// @return Returns true if the task state changed from READY to
  ///         RUNNING.  Otherwise, false.
  inline bool set_state_running(bool& in_utcache); //see internal/group.hh

  /// Sets the taks state to COMPLETED, regardless of which state the
  /// task is on.
  ///
  /// @param mem_order Memory synchronization ordering for the
  /// operation. The default is std::memory_order_seq_cst
  void set_state_completed() {

    MARE_INTERNAL_ASSERT(get_state().is_running(),
                         "Can't set task %p state to COMPLETED because its "
                         "state is not RUNNING: %s",
                         this, to_string().c_str());

    // No need to use a CAS here. We only call this method once for the task
    _state.store(task_state::COMPLETED, std::memory_order_seq_cst);
  }

  /// Sets the taks state to CANCELED, regardless of which state the
  /// task is on.
  ///
  /// @param mem_order Memory synchronization ordering for the
  /// operation. The default is std::memory_order_seq_cst
  void set_state_canceled(std::memory_order mem_order =
                          std::memory_order_seq_cst) {

    MARE_INTERNAL_ASSERT(!get_state().is_completed(),
                         "Can't set task %p state to CANCELED because it's "
                         "already COMPLETED: %s.",
                         this, to_string().c_str());

    MARE_INTERNAL_ASSERT(!get_state().is_canceled(),
                         "Can't set task %p state to CANCELED because it's "
                         "already COMPLETED: %s.",
                         this, to_string().c_str());

    // No need to use a CAS here. We only call this method once for the task
    _state.store(task_state::CANCELED, mem_order);
  }

  /// Marks the task as being in the unlaunched task cache.
  ///
  /// @param mem_order Memory synchronization ordering for the
  /// operation. The default is std::memory_order_seq_cst.
  ///
  /// @return
  /// true - The task transitioned to IN_UTCACHE.\n
  /// false - The task did not transitioned to IN_UTCACHE.
  bool set_state_in_utcache(std::memory_order mem_order =
                            std::memory_order_seq_cst) {
    task_state previous_state = _state.fetch_or(task_state::IN_UTCACHE,
                                                mem_order);
    return !previous_state.in_utcache();
  }

  /// @brief Transitions tasks state to cancelation requested.
  ///
  /// @param[out] state Task state after the transition to cancelation
  ///                   requested
  ///
  /// @param[in] mem_order Memory synchronization ordering for the
  ///            operation. The default is std::memory_order_seq_cst.
  ///
  /// @return
  /// true  - The task transitioned to cancelation requested. \n
  /// false - The task did not transition to cancelation reqeuested because:
  ///           - someone else had requested its cancelation first.
  ///           - the task had executed already.
  bool set_state_request_cancel(task_state& state,
                                std::memory_order mem_order =
                                std::memory_order_seq_cst) {
    state = _state.fetch_or(task_state::CANCEL_REQ, mem_order);
    if (state.is_marked_for_cancelation() || state.is_done())
      return false;

    state = state.get_raw_value() | task_state::CANCEL_REQ;
    return true;
  }

  /// @brief Marks the task as launched
  ///
  /// @param[out] state Task state after the transition to launched.
  ///
  /// @param[in] mem_order Memory synchronization ordering for the
  ///            operation. The default is std::memory_order_seq_cst.
  ///
  /// @return
  /// true  - The task transitioned from unlaunched to launched. \n
  /// false - The task was already launched.
  bool set_state_launched(task_state& state, std::memory_order mem_order =
                          std::memory_order_seq_cst) {

    state = _state.fetch_and(task_state::UNLAUNCHED_MASK, mem_order);
    if (state.is_launched())
      return false;

    state = state.get_raw_value() & task_state::UNLAUNCHED_MASK;
    return true;
  }

  /// @brief Adds a dependence between pred and tasks succ without locking
  /// pred.
  ///
  /// @param pred task to execute first.
  /// @param succ task to execute second.
  static inline void add_task_dependence_unlocked(task* pred, task* succ);

  /// @brief Makes task abandon its grups
  void leave_groups();

  static uint32_t get_initial_state(task_attrs& a) {
    if (has_attr(a, attr::anonymous))
      return 0;
    return task_state::UNLAUNCHED;
  }

  friend class test::task_tests;
};


template<typename PredTask, typename SuccTask> void
task::add_task_dependence_dispatch(PredTask& pred, SuccTask& succ)
{
  internal::task* pred_ptr = internal::c_ptr(pred);
  internal::task* succ_ptr = internal::c_ptr(succ);

  MARE_API_ASSERT(pred_ptr, "null task_ptr pred.");
  MARE_API_ASSERT(succ_ptr, "null task_ptr succ.");


  // succ is shared, so it is possible that it has been launched
  // already. Because we don't allow to add a predecessor to a task
  // that has been launched, we have to pay the penalty and do an
  // atomic load
  MARE_API_ASSERT(!succ_ptr->get_state().is_launched(),
                  "Successor task %p has already been launched: %s",
                  succ_ptr,
                  succ_ptr->to_string().c_str());

  add_task_dependence(pred_ptr, succ_ptr);

  MARE_API_DETAILED_ASSERT(!succ_ptr->get_state().is_launched(),
                           "Successor task %p has been launched while "
                           "setting up dependences: %s",
                           succ_ptr,
                           succ_ptr->to_string().c_str());
}


inline void
task::add_task_dependence_unlocked(task* pred, task* succ)
{
  MARE_API_DETAILED_ASSERT(succ, "null succ pointer");
  MARE_API_DETAILED_ASSERT(!succ->get_state().is_launched(),
                           "Successor task %p has been launched already.",
                           succ);

  MARE_API_DETAILED_ASSERT(pred, "null pred pointer");
  MARE_API_DETAILED_ASSERT(!pred->get_state().is_launched(),
                           "Predecessor task %p has been launched already.",
                           pred);

  auto current_state = pred->get_state();
  if (current_state.is_marked_for_cancelation()) {
    succ->cancel();
    return;
  }

  pred->_successors.push_back(succ);
  succ->predecessor_added();
  log::log_event(log::events::task_after(pred, succ));
}



/// Deletes task or decreases its reference counter,
/// depending on whether the task is reference counted.
///
/// @param t Task to relaase.
inline void
release_ownership(task* t) {
  MARE_INTERNAL_ASSERT(t, "trying to release ownership for invalid task "
                       "pointer");
  if (t->is_runtime_owned()) {
    delete t;
  } else {
    // removes one task sharer and deletes task if neccesary
    explicit_unref(t);
  }
}


inline void
task::execute_inline(group* g_ptr = nullptr) {
  task_state updated_state;

  // launch
  set_state_launched(updated_state);
  // caller holds reference, runtime does not own task
  if (g_ptr != nullptr)
    join_group(g_ptr, internal::task::utcache_policy::do_nothing);

  // scheduler::schedule_task
  // optimization: if we know parent task already, we can avoid calling
  //               current_task() again here. Example case: inside pfor.
  if (auto parent = current_task()) {
    if (auto parent_sched = parent->get_scheduler()) {
      auto acquired = request_ownership(*parent_sched);
      MARE_INTERNAL_ASSERT(acquired,
        "scheduler failed to acquire ownership of inline task %p", this);
    }
  }
  MARE_INTERNAL_ASSERT(updated_state.is_launched(),
                       "Cannot execute task %p inline "
                       "because it has been launched already: %s.",
                       this, to_string().c_str());
  schedule();
}

/// \brief Yields from the current task to the scheduler
///
/// \par
/// This function has no effect if called outside of a MARE task.
///
/// \par
/// NOTE: The ability to yield may go away in future releases of MARE.
inline void
yield()
{
  if (auto task = current_task()) {
    auto t_ptr = c_ptr(task);
    MARE_API_ASSERT(t_ptr, "null task_ptr");
    t_ptr->yield();
  }
}

};//namespace internal
};//namespace mare

