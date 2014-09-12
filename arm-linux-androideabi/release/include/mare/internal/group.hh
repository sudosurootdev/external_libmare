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
#include <forward_list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <mare/common.hh>
#include <mare/exceptions.hh>
#include <mare/internal/concurrentdensebitmap.hh>
#include <mare/internal/lattice.hh>
#include <mare/internal/log/log.hh>
#include <mare/internal/macros.hh>
#include <mare/internal/mareptrs.hh>
#include <mare/internal/task.hh>

namespace mare {

// forward declarations
void launch_and_reset(task_ptr&);

namespace internal {

class group_factory;

namespace test {
class group_tester;
} // namespace tester

class meet_db;

class group : public internal::ref_counted_object<group> {

  struct group_id {
    typedef size_t type;
  };

  class group_id_generator {

    static concurrent_dense_bitmap s_bmp;

  public:

    static constexpr group_id::type default_meet_id = ~group_id::type(0);

    static size_t get_num_leaves() {
      return (s_bmp.popcount());
    }

    static group_id::type get_leaf_id() {
      return (s_bmp.set_first());
    }

    static group_id::type get_meet_id() {
      return default_meet_id;
    }
    static void release_id(group_id::type id) {
      if (id == default_meet_id)
        return;
      s_bmp.reset(id);
    }

    friend class test::group_tester;
  }; // group_id_generator

public:

  typedef std::forward_list<group*> children_list;
  typedef std::forward_list<group_ptr> parent_list;

  /// Constant to mark that the lattice counters may required update
  static const size_t g_mark_bit =
    static_cast<size_t>(1) << ((sizeof(size_t) * 8) - 1);

  /// Largest group order
  static const size_t s_max_order = 200;

  /// group name.
  const std::string _name;

  /// Number of tasks in the group
  std::atomic<size_t> _tasks;

  /// Group parents and children
  parent_list _parents;
  children_list _children;

  /// _group_id and _order could be obtained from _bitmap.
  /// However, we access them quite often, so for now we
  /// are caching them here.

  /// Group id
  const group_id::type _group_id;

  /// Group bitmap
  const group_signature _bitmap;

  /// Number of bits in the bitmap.
  const  size_t _order;

  /// Task launched when _tasks reaches 0, and lock protecting it
  std::mutex _mutex;
  task_ptr _trigger_task;

  // Meed database is the list owned by the
  // contributing leaf with smallest id.
  meet_db* _meet_db;

  const log::group_id _log_id;

  // True if the user called task::cancel()
  // By default, _canceled is false. Once it transitions to true,
  // it can never go back to false.
  std::atomic<bool> _canceled;
  bool _parent_sees_tasks;

private:

  //Constructor for leaf groups
  group(std::string const& name):
    _name(name),
    _tasks(0),
    _parents(),
    _children(),
    _group_id(group_id_generator::get_leaf_id()),
    _bitmap(_group_id, sparse_bitmap::singleton),
    _order(1),
    _mutex(),
    _trigger_task(nullptr),
    _meet_db(nullptr),
    _log_id(),
    _canceled(false),
    _parent_sees_tasks(false) {

    log::log_event(log::events::group_created(this));

    MARE_INTERNAL_ASSERT(_group_id != group_id_generator::default_meet_id,
                         "Too many alive leaf groups: %zu", _group_id);
  }

  //Constructor for leaf groups
  group(const char* name):
    _name(name),
    _tasks(0),
    _parents(),
    _children(),
    _group_id(group_id_generator::get_leaf_id()),
    _bitmap(_group_id, sparse_bitmap::singleton),
    _order(1),
    _mutex(),
    _trigger_task(nullptr),
    _meet_db(nullptr),
    _log_id(),
    _canceled(false),
    _parent_sees_tasks(false) {

    log::log_event(log::events::group_created(this));

    MARE_INTERNAL_ASSERT(_group_id != group_id_generator::default_meet_id,
                         "Too many alive leaf groups: %zu", _group_id);
  }

  //constructor for meet groups
  group(group_signature& bitmap, meet_db* db) :
    _name(), // For efficiency reason, we assume meets have empty names
    _tasks(0),
    _parents(),
    _children(),
    _group_id(group_id_generator::get_meet_id()),
    //WARNING: Beyond this point never use bitmap (instead use _bitmap)
    _bitmap(std::move(bitmap)),
    _order(_bitmap.popcount()),
    _mutex(),
    _trigger_task(nullptr),
    _meet_db(db),
    _log_id(),
    _canceled(false),
    _parent_sees_tasks(false) {

    log::log_event(log::events::group_created(this));

    MARE_INTERNAL_ASSERT(_group_id == group_id_generator::default_meet_id,
                         "Invalid group_id for meet group: %zu", _group_id);

    MARE_API_ASSERT(_order <= s_max_order,
                    "Too many groups intersected. Max is %zu.", s_max_order);
  }

  //constructor for pfor meet groups
  group(group* parent) :
    _name(),
    _tasks(0),
    _parents(),
    _children(),
    _group_id(group_id_generator::get_meet_id()),
    _bitmap(), // Bitmap will be empty for pfor group
    // For now we can assume order of zero represents parent-less pfor!
    _order((parent == nullptr)?0:parent->get_order() + 1),
    _mutex(),
    _trigger_task(nullptr),
    _meet_db(nullptr),
    _log_id(),
    _canceled(false),
    _parent_sees_tasks(false) {

    log::log_event(log::events::group_created(this));

    MARE_INTERNAL_ASSERT(_group_id == group_id_generator::default_meet_id,
                         "Invalid group_id for meet group: %zu", _group_id);

    MARE_API_ASSERT(_order <= s_max_order,
                    "Too many groups intersected. Max is %zu.", s_max_order);

    // If parent is null, no update to the parent or children list is needed
    if (parent != nullptr) {
      _parents.push_front(group_ptr(parent));

      // we have to acquire the lock because we are changing the lattice
      std::lock_guard<lattice::lock_type> lattice_lock(lattice::mutex());
      parent->_children.push_front(this);
    }

  }

  // Disable all copying and movement, since other objects may have
  // pointers to the internal fields of this object.
  MARE_DELETE_METHOD(group(group const&));
  MARE_DELETE_METHOD(group(group&&));
  MARE_DELETE_METHOD(group& operator=(group const&));
  MARE_DELETE_METHOD(group& operator=(group&&));

public:

  /** Returns the log id of the group object.
      @return log::group_id
  */
  const log::group_id get_log_id() const {
    return _log_id;
  }

  /// Returns the group name
  /// @return const reference to the name of the group.
  std::string const& get_name() const {
    return _name;
  }

  /// Describes group. Useful for debugging.
  /// @return String that describes the group status.
  std::string to_string(std::string const& msg="") const;

  void wait();

  /// Cancels all tasks in the group and prevents any new tasks from
  /// being added to it. There might be tasks in the group running after
  /// cancel() returns. Eventually, all tasks will get removed from
  /// the group. In order to make sure that the group is empty, you
  /// must call group::wait() after group::cancel().
  void cancel();


  /// Cancels task if it belongs to this group.
  ///
  /// @param task task to cancel
  /// @return true of task was cancelled, false otherwise
  bool cancel_member(task* task) const {
    MARE_INTERNAL_ASSERT(task, "invalid task pointer checked for group "
                         "membership in %p", this);
    auto grp = task->get_group();
    if (grp && this->is_ancestor_of(grp)) {
      task->cancel();
      return true;
    }
    return false;
  }

  /// Returns true if the group is canceled.
  /// @return true if the group is canceled. Otherwise, false.
  bool is_canceled(std::memory_order mem_order=std::memory_order_seq_cst) {
    return _canceled.load(mem_order);
  }

  ///@brief Returns the bitmap corresponding to the group's index in the cache
  ///@return Bitmap corresponding to the group's index in the cache
  group_signature const& get_signature() const {
    return _bitmap;
  }

  ///@brief Returns the number of ones in the group's bitmap
  size_t get_order() const {
    return _order;
  }

  bool is_empty(std::memory_order mem_order=std::memory_order_seq_cst) const {
    auto num_tasks = _tasks.load(mem_order);
    return (num_tasks == 0);
  }

  meet_db* get_meet_db() {return _meet_db;};

  /// Creates the meet list on demand when there is a new meet
  /// that will end up in this list
  void set_meet_db(meet_db* db) {
    MARE_INTERNAL_ASSERT(_meet_db == nullptr,
        "Meet database must be null inorder to be created");
    _meet_db = db;
  };

  /// Increases the task counter in g by 1. If it's the first task added
  /// to the group, then we also increase the reference counting to simulate
  /// that the runtime has a shared pointer to it. If the group is not a leaf,
  /// it grabs the lattice lock and propagates the counter increase to
  /// its ancestors.
  inline void inc_task_counter(lattice_lock_policy p = acquire_lattice_lock);
  void inc_task_counter_meet(lattice_lock_policy p =
      acquire_lattice_lock);

  /// Decreases the task counoter in g by 1. If the counter reaches 0,
  /// the shared_ptr to self in the group is reset.
  inline void dec_task_counter(lattice_lock_policy p = acquire_lattice_lock);
  void dec_task_counter_meet(lattice_lock_policy p =
      acquire_lattice_lock);

  /// @brief Checks whether this group is an ancestor of group g
  /// Group g1 is an ancestor of group g2 if g2's bitmap is a
  /// superset of g1's bitmap. Notice that any group is an ancestor
  /// of itself
  bool is_ancestor_of(group* g) const {
    MARE_INTERNAL_ASSERT(g, "nullptr group");
    return (g->_bitmap.subset(_bitmap));
  }

  /// @brief Checks whether this group is an strict ancestor of group g
  /// Group g1 is an ancestor of group g2 if g2's bitmap is a
  /// superset of g1's bitmap and g1 is different than g2.
  bool is_strict_ancestor_of(group* g) const {
    MARE_INTERNAL_ASSERT(g, "nullptr group");
    return (is_ancestor_of(g) && (this != g));
  }

  /// @brief Checks whether the group is a leaf in a group tree
  bool is_leaf() const {
    return get_id() != group_id_generator::default_meet_id;
  }

  /// @brief Checks whether the group is a pfor (a special meet)
  /// For now we assume empty group signature indicates its pfor
  bool is_pfor() const {
    return _bitmap.is_empty();
  }

  /// @brief Checks whether the group is pfor and has no parent
  /// For pfor groups, this guarantees that the parent list will
  /// remain empty forever! Note: for a normal meet the parent
  // list maybe changed at any point!
  bool is_pfor_with_no_parent() const {
    // For performance, we might also be able to return (_order == 0)
    return is_pfor() && _parents.empty();
  }

  struct should_delete_functor {
    bool operator()(group*) {
      return true;
    }
  };

  ~group();

private:

  group_id::type get_id() const {
    return _group_id;
  }

  /// Same as cancel, except it does not acquire the lattice lock.
  void propagate_cancelation() {
    bool expected = false;
    if (std::atomic_compare_exchange_strong(&_canceled, &expected, true))
      for(auto &child : _children)
        child->propagate_cancelation();
  }

  ///-----------------Clean this one up---------
  void migrate_task_from(group* origin) {
    bool propagated_to_origin = false;
    propagate_task_migration(origin, propagated_to_origin);

    if (!propagated_to_origin)
      origin->dec_task_counter(do_not_acquire_lattice_lock);
  }

  void propagate_inc_task_counter() {
    MARE_INTERNAL_ASSERT(!is_leaf(),"Can't propagate task increase in leaf "
                         "group: %s", to_string().c_str());
    MARE_INTERNAL_ASSERT(!_parent_sees_tasks, "Can't propagate task increase "
                         "in group: %s", to_string().c_str());
    _parent_sees_tasks = true;
    for (auto& parent: _parents)
      c_ptr(parent)->inc_task_counter(do_not_acquire_lattice_lock);
  }

  void propagate_dec_task_counter() {
    MARE_INTERNAL_ASSERT(_parent_sees_tasks, "Can't propagate task decrease "
                         "in group: %s", to_string().c_str());
    _parent_sees_tasks = false;
    for (auto& parent: _parents)
      c_ptr(parent)->dec_task_counter(do_not_acquire_lattice_lock);
  }

  /// Increases the task counter in g by 1. If it's the first task added
  /// to the group, then we also increase the reference counting to simulate
  /// that the runtime has a shared pointer to it. If the group is not a leaf,
  /// it propagates the counter increase to its ancestors without grabing the
  /// lattice lock. It'll propagate it only until it finds the origin group.
  ///
  void propagate_task_migration(group* origin, bool& propgte_to_origin);

  static std::string get_meet_name();

  /// @return task_ptr for waiting on until the next time the group
  ///     becomes empty.  Returns nullptr if group is already empty.
  task_ptr ensure_trigger_task_unless_empty();

  inline task_ptr reset_trigger_task() {
    std::lock_guard<std::mutex> lock(_mutex);
    // Making sure a trigger task is only acquired when the group lock is
    // taken and the group is empty.
    // Although the task counter is checked in dec_task_counter before calling
    // launch_trigger task, it could be that competing tasks can be injected
    // into the group and this can lead to early launch of trigger task
    // before wait_for is reached, which can lead to pre-mature return of
    // wait_for! This is a common pattern in pfor_each codes.
    if (!is_empty())
      return nullptr;
    task_ptr t = std::move(_trigger_task);
    MARE_INTERNAL_ASSERT(!_trigger_task,
                         "std::move() did not reset group_mutex to nullptr");
    return t;
  }

  void launch_trigger_task() {
    if (auto t = reset_trigger_task()) {
      c_ptr(t)->cleanup_trigger_task();
    }
  }

public:
  /// @brief Returns the number of created leaves
  static size_t get_num_leaves() {
    return group_id_generator::get_num_leaves();
  };

  typedef test::group_tester tester;

  friend class lattice;
  friend class group_factory;
  friend class test::group_tester;
};// class group

// In order to keep track of all the previously created meets in the
// lattice, each meet is associated with a leaf owner and that owner leaf
// keeps track of its assigned meets in a list (meet database).
// During intersection operation, in lookup time (the find function),
// the list is traversed linearly until a signature hit is found.
// For a meet group, the owner group is the leaf with smallest id
// contributing to that meet. Assuming A, B ,C leaves with ids 0, 1, and 2
// DB owner of A & B & D = A, DB owner of B & C = B, DB owner of A & C = A
// We use a list instead of the map because in common cases, the number
// of meets assign to each leaf is small and the overhead of traversing
// the list is significantly small as a result of distributing meets
// among leaves.
class meet_db {
 public:
  typedef std::forward_list<group*> meet_list;
  meet_db(size_t leaf_id):
    _leaf_id(leaf_id),
    _meet_list() {};

  size_t get_leaf_id() const {return _leaf_id;};
  bool empty() {return _meet_list.empty();};

  // @brief Looks up a meet using its signature
  group* find(group_signature& sig) {
    MARE_INTERNAL_ASSERT(this != nullptr,
        "The group db should not be null!");
    for (auto& meet: _meet_list) {
      if (sig.get_hash_value() == meet->get_signature().get_hash_value()) {
        if (sig == meet->get_signature())
          return meet;
      }
    }
    return nullptr;
  }

  // @brief Adds a meet to its db
  void add(group* g) {
    MARE_INTERNAL_ASSERT(this != nullptr,
        "The group db should not be null!");
#ifndef NDEBUG
    s_meet_groups_in_db++;
#endif
    _meet_list.push_front(g);
  }

  // @brief Removes a meet from its db
  void remove(group* g) {
    MARE_INTERNAL_ASSERT(this != nullptr,
        "The group db should not be null!");
#ifndef NDEBUG
    s_meet_groups_in_db--;
#endif
    _meet_list.remove(g);
  }

  MARE_DELETE_METHOD(meet_db(meet_db const&));
  MARE_DELETE_METHOD(meet_db(meet_db&&));
  MARE_DELETE_METHOD(meet_db& operator=(meet_db const&));
  MARE_DELETE_METHOD(meet_db& operator=(meet_db&&));
#ifndef NDEBUG
// Only used for the debug tests
static size_t meet_groups_in_db() {return s_meet_groups_in_db.load();};
static std::atomic<size_t> s_meet_groups_in_db;
#else
static size_t meet_groups_in_db() {return 0;};
#endif

private:
  // The leaf id of the leaf group holding this list
  // Storing leaf id here might seem redundant but can
  // potentially speedup finding the leave with smallest id
  // An alternative is to use ffs to find the lowest set bit
  // in a and b bitmaps.
  size_t _leaf_id;
  // The list of meets tracked in this list
  meet_list _meet_list;
friend class test::group_tester;
};

// This class manages all group constructions for different group types
class group_factory
{
  public:
  // @brief Creates a leaf group using an input bitmap
  static group* create_leaf_group(std::string const& name) {
    return new group(name);
  }

  // @brief Creates a leaf group using an input bitmap
  static group* create_leaf_group(const char* name) {
    return new group(name);
  }

  // @brief Creates a meet group using an input bitmap.
  static group* create_meet_group(group_signature& bitmap, meet_db* owner_db) {
    return new group(bitmap, owner_db);
  }

  // @brief Creates a pfor group as a children of the input group and
  // returns a pfor group_ptr to it.
  static group* create_pfor_group(group* const& parent_group) {
    return new group(parent_group);
  }

};

inline void
group::inc_task_counter(lattice_lock_policy lock_policy)
{
  if (is_leaf() || is_pfor_with_no_parent()) {
    if (_tasks.fetch_add(1, std::memory_order_seq_cst) != 0) {
      // No 0->1 transition happened
      return;
    }
    explicit_ref(this);
    // To make sure inc/dec ordering of tasks is always observed!
    // If a task is just injected an not executed yet, task counter will
    // never be zero
    MARE_INTERNAL_ASSERT(!is_empty(), "Group must not be empty");
    return;
  }
  inc_task_counter_meet(lock_policy);
}


inline void
group::dec_task_counter(lattice_lock_policy lock_policy)
{
  if (is_leaf() || is_pfor_with_no_parent()) {
    if (_tasks.fetch_sub(1, std::memory_order_seq_cst) != 1) {
      // No 1->0 transition happened
      return;
    }
    launch_trigger_task();
    explicit_unref(this);
    return;
  }
  dec_task_counter_meet(lock_policy);
}

inline void
task::leave_groups()
{
  auto g = _group.load();
  if(!g)
    return;

  g->dec_task_counter();
  _group.store(nullptr);
  MARE_INTERNAL_ASSERT(_group.load() == nullptr,
                       "Found bug in GCC 4.6.2 std::atomic<> where atomic "
                       "store does not work for pointer types on some "
                       "platforms");
}

inline bool
task::is_canceled() const
{
  if(!is_cancelable())
    return false;

  // Lock the task to protect us from the case where the
  // tasks completes execution and removes itself from
  // the group(s)

  std::lock_guard<std::mutex> task_lock(_mutex);

  auto current_state = get_state();

  if (current_state.is_running() || current_state.is_completed())
    return false;

  if (current_state.is_canceled())
    return true;

  // By now we know that when we read current_state, the task wasn't
  // running. If it was marked for cancelation, then we know for sure
  // that the task will never execute and we can return true.

  if (current_state.is_marked_for_cancelation())
    return true;

  // By now we know that when we read current_state, the task wasn't
  // running and it wasn't marked for cancelation. Because we grab the
  // lock, we can guarantee that the task is, at most, in running state
  // (remember that transitioning from RUNNING to COMPLETED or CANCELED
  // requires that we grab the task lock).

  if (auto g = _group.load())
    return g->is_canceled();

  return false;
}

inline void
task::prepare_for_ut_cache(group* g, std::memory_order mem_order) {
  bool success = set_state_in_utcache();
  MARE_INTERNAL_ASSERT(success, "Tasks already marked as in utcache");
  g->inc_task_counter();
  _group.store(g, mem_order);
}

inline bool
task::set_state_running(bool& in_utcache)
{
  MARE_INTERNAL_ASSERT(get_state().get_num_predecessors() ==0,
                       "Can't transition task %p to RUNNING because it has "
                       "predecessors: %s.",
                       this, to_string().c_str());

  MARE_INTERNAL_ASSERT(get_state().is_launched(),
                       "Can't transition task %p to RUNNING because is not "
                       "launched yet: %s.",
                       this, to_string().c_str());

  // In the common case, the task has no predecessors, it's not marked
  // for cancelation, and is not in the unlaunched task cache.
  task_state::type expected = 0;
  if (atomic_compare_exchange_strong_explicit(&_state,
                                              &expected,
                                              task_state::RUNNING,
                                              std::memory_order_seq_cst,
                                              std::memory_order_seq_cst))
    return true;

  // It seems that we the task wasn't in the state we expected.  If
  // the state wasn't 0 was because the task is in the unlaunched task
  // cache or is marked for cancelation. If it's marked for
  // cancelation, then we must not transition to RUNNING
  task_state curr_state(expected);

  if (curr_state.in_utcache())
    in_utcache = true;

  if (curr_state.is_marked_for_cancelation())
    return false;

  expected = task_state::IN_UTCACHE;

  // The task had not been marked for cancelation, but it was in the
  // unlaunched task_cache
  if (atomic_compare_exchange_strong_explicit(&_state,
                                              &expected,
                                              (task_state::RUNNING |
                                               task_state::IN_UTCACHE),
                                              std::memory_order_seq_cst,
                                              std::memory_order_seq_cst))
    return true;

  MARE_INTERNAL_ASSERT(task_state(expected).is_marked_for_cancelation(),
                       "Unexpected task state: %s", to_string().c_str());
  return false;
}

inline void
task::abort_on_cancel()
{
  // This is a simplifed version of is_canceled because
  // we more or less know the state of the task

  auto current_state = get_state();
  MARE_INTERNAL_ASSERT(current_state.is_running(),
                       "Task %p is not running: %s",
                       this, to_string().c_str());

  if (current_state.is_marked_for_cancelation())
    throw mare::abort_task_exception();

  auto g = _group.load();
  if (g && g->is_canceled())
    throw mare::abort_task_exception();
}

} //namespace internal

} //namespace mare

#include <mare/internal/log/imloggerevents.hh>
