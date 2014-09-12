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

#include <array>
#include <atomic>
#include <fstream>
#include <memory>
#include <string>

#include <mare/internal/debug.hh>
#include <mare/internal/functiontraits.hh>
#include <mare/internal/group.hh>
#include <mare/internal/log/log.hh>
#include <mare/internal/macros.hh>

namespace mare {

namespace internal {

// forwarding declaration
namespace test {
  class pool_tests;
} //namespace test

enum class try_steal_result : uint8_t {
  SUCCESS          = 0,
  ALREADY_FINISHED = 1,
  ALREADY_STOLEN   = 2,
  NULL_POINTER     = 3,
  INVALID          = 4,
};

class ws_tree;

/**
This class implements the basic work unit of an adaptive worksteal tree.
When balancing pfor workloads amongst N tasks, the complete working range
is adaptively splitted into different sizes of the work units. The adaptive
splitting process is organized as a tree structure.

Because we pre-allocate a pool of tree nodes on the tree, each node
has a pointer to the tree and deletegate construction to the tree.
*/

class ws_node {

public:
  typedef uint16_t       counter_type;
  typedef size_t         size_type;
  typedef ws_node        node_type;
  typedef ws_tree        tree_type;

  static const size_t   UNCLAIMED;
  static const size_t   STOLEN;
  static const intptr_t NODE_COMPLETE_BIT;

private:
  typedef std::atomic<size_type>            atomic_size_type;
  typedef ws_node*                          node_type_pointer;
  typedef ws_tree*                          tree_type_pointer;
  typedef std::atomic<node_type_pointer>    atomic_node_type_pointer;

#ifdef ADAPTIVE_PFOR_DEBUG
  atomic_size_type _traversal;
  size_type _worker_id;
  size_type _progress_save;
#endif

  atomic_node_type_pointer _left;
  atomic_node_type_pointer _right;
  counter_type _left_traversal;
  counter_type _right_traversal;
  const size_type _first;
  const size_type _last;
  atomic_size_type _progress;
  tree_type_pointer _tree;

  ws_node(size_type first, size_type last, size_type progress,
          tree_type_pointer tree) :
#ifdef ADAPTIVE_PFOR_DEBUG
    _traversal(0),
    _worker_id(0),
    _progress_save(0),
#endif // ADAPTIVE_PFOR_DEBUG
    _left(nullptr),
    _right(nullptr),
    _left_traversal(0),
    _right_traversal(0),
    _first(first),
    _last(last),
    _progress(progress),
    _tree(tree) {

  }

  friend class ws_tree;
  friend class test::pool_tests;

  // Disable all copying and movement.
  MARE_DELETE_METHOD(ws_node(ws_node const&));
  MARE_DELETE_METHOD(ws_node(ws_node&&));
  MARE_DELETE_METHOD(ws_node& operator=(ws_node const&));
  MARE_DELETE_METHOD(ws_node& operator=(ws_node&&));

public:

  // will be required for node_slab, gcc-4.6 will report erorr if call
  // ws_node(0,0,0,nullptr) in member initialization list
  ws_node() :
#ifdef ADAPTIVE_PFOR_DEBUG
    _traversal(0),
    _worker_id(0),
    _progress_save(0),
#endif // ADAPTIVE_PFOR_DEBUG
    _left(nullptr),
    _right(nullptr),
    _left_traversal(0),
    _right_traversal(0),
    _first(0),
    _last(0),
    _progress(0),
    _tree(nullptr) {

  }

#ifdef ADAPTIVE_PFOR_DEBUG
  size_type count_traversal() const {
    return _traversal.load(std::memory_order_relaxed);
  }
  void increase_traversal(size_type n,
                          std::memory_order order = std::memory_order_relaxed)
  {
    _traversal.fetch_add(n, order);
  }

  size_type get_worker_id() const { return _worker_id; }
  void set_worker_id(size_type id){ _worker_id = id; }

  size_type get_progress_save() const { return _progress_save; }
  void set_progress_save(size_type psave) { _progress_save = psave; }
#endif // ADAPTIVE_PFOR_DEBUG

  node_type* get_left(std::memory_order order = std::memory_order_relaxed)
  { return _left.load(order); }
  node_type* get_right(std::memory_order order = std::memory_order_relaxed)
  { return _right.load(order); }

  bool is_unclaimed(std::memory_order order = std::memory_order_relaxed) {
    return get_progress(order) == UNCLAIMED;
  }

  bool is_stolen(size_type blk_size,
                 std::memory_order order = std::memory_order_relaxed) {
    auto progress = get_progress(order);
    return (progress == STOLEN) || (progress == STOLEN + blk_size);
  }

  void set_left(node_type_pointer node,
                std::memory_order order = std::memory_order_relaxed)
  { _left.store(node, order); }
  void set_right(node_type_pointer node,
                 std::memory_order order = std::memory_order_relaxed)
  { _right.store(node, order); }

  size_type get_first() const { return _first; }
  size_type get_last() const { return _last; }


  size_type get_progress(std::memory_order order = std::memory_order_relaxed) {
    return _progress.load(order);
  }

  size_type inc_progress(size_type n, std::memory_order order) {
    return _progress.fetch_add(n, order);
  }

  size_type get_left_traversal() const { return _left_traversal; }
  size_type get_right_traversal() const { return _right_traversal; }

  void inc_left_traversal()  { _left_traversal++; }
  void inc_right_traversal() { _right_traversal++; }

  // Set rightmost bit for tree pointer to 0
  // Used to check completion of a leaf node.
  tree_type_pointer get_tree() const {
    return reinterpret_cast<tree_type_pointer>(
        reinterpret_cast<intptr_t>(_tree) & (~NODE_COMPLETE_BIT));
  }

  void set_completed() {
    _tree = reinterpret_cast<tree_type_pointer>(
        reinterpret_cast<intptr_t>(_tree) | NODE_COMPLETE_BIT);
  }

  bool is_completed() {
    return
      (reinterpret_cast<intptr_t>(_tree) & NODE_COMPLETE_BIT)
        == NODE_COMPLETE_BIT;
  }

  // every node in work steal tree must have two children
  // so only checking left child is sufficient.
  bool is_leaf() { return get_left() == nullptr; }

  // Attempts to claim ownership of a worksteal tree node.
  // One can only call this operation on a UNCLAIMED node.
  // Return true if the claim is successful and false otherwise.
  static bool try_own(node_type* n);

  // Attempts to steal from an OWNED worksteal tree node.
  // A node cannot be stolen if it's in state UNCLAIMED,
  // STOLEN (someone succeeds in stealing before you), or
  // the workload on the node was finished already. If
  // stealing is successful, the worker task is responsible
  // for expanding the node and work on the right child.
  static try_steal_result try_steal(node_type* n, const size_type blk_size);

}; // class ws_node

/**
This class implements a thread-safe pool for objects of class T. The
pool is implemented as a linked list of slabs. Each slab allocates
Size_ objects. When a slab gets full, it creates a new one that gets
added to the list.

This class makes the following assumptions:

- Objects are never deleted (thus the pool can only grow)

- The slab size must be larger than the number of concurrent threads
accesing the pool (MAX_SHARERS). This allows us to make some
assumptions and make the pool faster */

// Will get rid of template here if MSVC12 allows for constexpr
template<size_t Size_>
class ws_node_pool {

public:

  typedef ws_node    node_type;
  typedef size_t     size_type;
  typedef node_type* pointer;

private:

  /**
     A slab is a bunch of tree nodes and
     a pointer to the next slab
  */
  class node_slab {

    // @todo Make this a char* array so we don't have to pay
    // for construction

    node_type _buffer[Size_];
    const size_type _first_pos;
    std::atomic<node_slab*> _next;

  public:

    // Constructor
    node_slab(size_type first_pos):
      _first_pos(first_pos),
      _next(nullptr) {
    }

    size_type begin() const { return _first_pos; }
    size_type end() const { return begin() + Size_; }

    bool has(size_type pos) const {
      return ((pos >= begin()) && (pos < end()));
    }

    void set_next(node_slab* next, std::memory_order order) {
      _next.store(next, order);
    }

    node_slab* get_next(std::memory_order order) {
      return _next.load(order);
    }

    pointer get_ptr(size_type pos) {
      return &_buffer[pos];
    }

    // Disable all copying and movement.
    MARE_DELETE_METHOD(node_slab(node_slab const&));
    MARE_DELETE_METHOD(node_slab(node_slab&&));
    MARE_DELETE_METHOD(node_slab& operator=(node_slab const&));
    MARE_DELETE_METHOD(node_slab& operator=(node_slab&&));

    ~node_slab() {}
  }; // class node_slab


  node_slab _inlined_slab;
  std::atomic<size_type> _pos;
  std::atomic<node_slab*> _slab;

  // Returns slab that includes pos, or nullptr if none
  node_slab* find_slab(const size_type pos, node_slab* first_slab) {
    auto ck = first_slab;
    while(ck != nullptr) {
      if (ck->has(pos))
        return ck;
      ck = ck->get_next(std::memory_order_acquire);
    }
    return nullptr;
  }

  // This method creates a new slab and attaches it to the slab linked list
  node_slab* grow(size_type pos, node_slab* current_slab) {

    MARE_INTERNAL_ASSERT(current_slab, "null ptr");

    // Corner case. I'd doubt that it would ever execute
    if (pos == 0)
      return get_inlined_slab();

    // Allocate new slab
    node_slab* new_slab = new node_slab(pos);

    // We hope that current_slab is up_to_date and that
    // it is the right predecessor. If not, go find the
    // right predecessing slab.
    size_type current_end = current_slab->end();
    if (current_end != pos) {
      current_slab = find_slab(pos - 1, get_inlined_slab());
      MARE_INTERNAL_ASSERT(current_slab != nullptr, "null ptr");
    }

    MARE_INTERNAL_ASSERT(current_slab->has(pos - 1), "Wrong slab");

    // Update next pointer
    current_slab->set_next(new_slab, std::memory_order_relaxed);
    // Store new slab. Notice that we are using release to write the
    // new slab pointer. This guarantees that old_slab->_next is
    // updated before _slab becomes visible.
    _slab.store(new_slab, std::memory_order_release);
    log::log_event(log::events::ws_tree_new_slab());
    return new_slab;
  }

  pointer get_next_impl() {

    // We first read both pos and slab. We can use relaxed
    // atomics for both because if we read the wrong slab
    // we'll be able to detect it and fix it
    auto pos = _pos.fetch_add(1U, std::memory_order_relaxed);
    auto slab = _slab.load(std::memory_order_relaxed);

    // This is the position within the slab. Notice that this
    // position is the same for all slabs
    const auto pos_in_slab = pos % Size_;

    // _slab can't be NULL because we initialize it with
    // the address of the inlined slab.
    MARE_INTERNAL_ASSERT(slab != nullptr, "Invalid ptr null");

    // This is the common case: the current pos is within the
    // boundaries of the slab we observed
    if (slab->has(pos))
      return slab->get_ptr(pos_in_slab);

    // So something is not what we expected. Perhaps we need
    // a new slab because we ran out of room in the current one
    // or we read an old slab. In any case, we read the most
    // up-to-date slab.
    slab = _slab.load(std::memory_order_acquire);

    if (pos_in_slab == 0){
      // We have no room left in the current slab, and since we are
      // the first ones to figure this out, we are in charge of
      // growing the pool. Notice that at this point _slab cannot
      // change because of our rule that limits the number of
      // concurrent accesses.
      return grow(pos, slab)->get_ptr(0);
    }

    // We now try to find our slab. We might have to spin a little
    // bit if the slab we are supposed to return is not there yet.
    do {
      slab = find_slab(pos, get_inlined_slab());
    } while (!slab);

    MARE_INTERNAL_ASSERT(slab->has(pos), "Wrong slab");

    return slab->get_ptr(pos_in_slab);
  }

public:

  ws_node_pool(size_type max_sharers) :
    _inlined_slab(0),
    _pos(0),
    _slab(&_inlined_slab) {
    MARE_API_ASSERT(max_sharers < Size_, "Too many sharers for pool.");
  }

  //  Returns pointer to the next available memory location.
  //  This is thread safe. If there are no more memory available
  //  available in the current slab, it'll create a new one
  pointer get_next() {
    auto next = get_next_impl();
    MARE_API_ASSERT(next, "Could not allocate slab.");
    log::log_event(log::events::ws_tree_node_created());
    return next;
  }

  // Returns pointer to inlined slab
  node_slab* get_inlined_slab() { return &_inlined_slab; }

  // Destructor.
  // Not thread-safe. It is the user's responsibility to
  // make sure that no clients are accessing it
  // while it runs
  ~ws_node_pool() {
    auto next_slab = _inlined_slab.get_next(std::memory_order_acquire);
    while (next_slab) {
      auto tmp = next_slab->get_next(std::memory_order_relaxed);
      delete next_slab;
      next_slab = tmp;
    }
  };

  // Disable all copying and movement.
  MARE_DELETE_METHOD(ws_node_pool(ws_node_pool const&));
  MARE_DELETE_METHOD(ws_node_pool(ws_node_pool&&));
  MARE_DELETE_METHOD(ws_node_pool& operator=(ws_node_pool const&));
  MARE_DELETE_METHOD(ws_node_pool& operator=(ws_node_pool&&));
}; //ws_node_pool


/**
  A worksteal tree is a binary tree representing how pfor workload
  is adaptively splitted amongst tasks. All the tree nodes are stored
  in a pre-allocated lock-free node pool defined in class ws_node_pool.
*/
class ws_tree {

  typedef size_t            size_type;
  typedef ws_node           node_type;
  // we use 256 here because the average number of tree
  // node for pfor micro-benchmarks is around 150. We
  // would like to use a power of 2 number and don't want
  // to create an extra slab in most cases.
  typedef ws_node_pool<256> node_pool_type;

public:

  ws_tree(size_t first, size_t last, size_t blk_size) :
    _max_tasks(internal::num_execution_contexts()),
    _node_pool(_max_tasks),
    // we initialize progress to first+blk_size for root
    // because root must be responsible for [first, first + blk_size).
    // Otherwise checking for ALREADY_STOLEN in try_steal
    // might fail.
    _root(new (get_new_node_placement())
          node_type(first, last, first + blk_size, this)),
    _prealloc_leaf(1),
    _prealloc_level(0),
    _blk_size(blk_size) {

  }

  node_type* get_root() const { return _root; }
  size_type get_max_tasks() const { return _max_tasks; }
  size_type get_leaf_num() const { return _prealloc_leaf; }
  size_type get_level() const { return _prealloc_level; }
  size_type get_blk_size() const { return _blk_size; }
  size_type range_start() const { return _root->get_first(); }

  // When we statically split workload and assign them to tasks in advance.
  static node_type* create_claimed_node(size_type first,
                                        size_type last,
                                        node_type* parent) {
    MARE_INTERNAL_ASSERT(parent != nullptr, "Invalid parent pointer.");

    ws_tree* tree = parent->get_tree();
    MARE_INTERNAL_ASSERT(tree != nullptr, "Invalid tree pointer.");
    auto* placement = tree->get_new_node_placement();
    return new (placement) node_type(first, last, first, tree);
  }

  // When we steal a node and start splitting, the left child
  // becomes uncaimed.
  static node_type* create_unclaimed_node(size_type first, size_type last,
                                          node_type* parent) {
    MARE_INTERNAL_ASSERT(parent != nullptr, "Invalid parent pointer.");

    ws_tree* tree = parent->get_tree();
    MARE_INTERNAL_ASSERT(tree != nullptr, "Invalid tree pointer.");
    auto* placement = tree->get_new_node_placement();
    return new (placement) node_type(first, last, node_type::UNCLAIMED, tree);
  }

  // When we steal a node and start splitting, the right child
  // becomes stolen
  static node_type* create_stolen_node(size_type first, size_type last,
                                       node_type* parent) {
    MARE_INTERNAL_ASSERT(parent != nullptr, "Invalid parent pointer.");
    MARE_INTERNAL_ASSERT(first != node_type::UNCLAIMED,
                         "Stolen node can't be unclaimed");

    // This is part of the speculative tree creation algorithm.
    // when a stealer successfully steals a right node
    // to work on, encourages future search to go for
    // the left subtree to "own" the leftover work
    parent->inc_right_traversal();

    ws_tree* tree = parent->get_tree();
    MARE_INTERNAL_ASSERT(tree != nullptr, "Invalid tree pointer.");
    auto* placement = tree->get_new_node_placement();
    return new (placement) node_type(first, last, first, tree);
  }

  // Statically split work on a pre-built tree.
  // Assuming the number of execution contexts is P, we create
  // the first L = ceil{log2(P)} levels of tree and assign the leaf
  // nodes to the first 2^{L} tasks. These new nodes are placed
  // inside the node pool as usual.
  void split_tree_before_stealing(size_t nctx);

  // if split tree before stealing, obtain the pre-assigned
  // leaf and return
  node_type* find_work_prebuilt(size_type tid);

  // Find work on tree rooted at n
  // When recursively seaching for work from n, we choose
  // left or right based on speculation, i.e., based on historical
  // record of direction selection outcome, the search will
  // choose the subtree with possiblly less nodes.
  // The key assumption is that the event of stealing or owning is
  // highly correlated to previous direction selection.
  node_type* find_work_intree(node_type* n, size_type blk_size);

private:

  node_type* get_new_node_placement() {
    return _node_pool.get_next();
  }

  const size_type _max_tasks;
  node_pool_type _node_pool;
  node_type* _root;
  size_type _prealloc_leaf;
  size_type _prealloc_level;
  size_type _blk_size;

  // Disable all copying and movement.
  MARE_DELETE_METHOD(ws_tree(ws_tree const&));
  MARE_DELETE_METHOD(ws_tree(ws_tree&&));
  MARE_DELETE_METHOD(ws_tree& operator=(ws_tree const&));
  MARE_DELETE_METHOD(ws_tree& operator=(ws_tree&&));
}; // class ws_tree

template<typename UnaryFn>
class adaptive_pfor_strategy
{
public:
  typedef size_t  size_type;
  typedef ws_tree tree_type;
  typedef ws_node work_item_type;
  typedef typename function_traits<UnaryFn>::f_type_in_task f_type;

  adaptive_pfor_strategy(group_ptr g,
                         size_type first,
                         size_type last,
                         UnaryFn&& f,
                         task_attrs attrs,
                         size_type blk_size) :
    _group(g),
    _workstealtree(first, last, blk_size),
    _f(f),
    _task_attrs(attrs),
    _prealloc(false) {

    }

  size_type get_max_tasks() const { return _workstealtree.get_max_tasks(); }
  size_type get_blk_size() const { return _workstealtree.get_blk_size(); }

  void static_split(size_type max_tasks) {
    _workstealtree.split_tree_before_stealing(max_tasks);
    _prealloc = true;
  }

  // we don't expose tree so we wrap find_work_prebuilt
  work_item_type* find_work_prebuilt(size_type task_id)
  {
    return _workstealtree.find_work_prebuilt(task_id);
  }

  // we don't expose tree so we wrap find_work_intree
  work_item_type* find_work_intree(work_item_type* n, size_type blk_size)
  {
    return _workstealtree.find_work_intree(n, blk_size);
  }

  work_item_type* get_root() const { return _workstealtree.get_root(); }
  group_ptr get_group() { return _group; }
  UnaryFn& get_unary_fn() { return _f; }
  bool is_prealloc() { return _prealloc; }
  size_type get_prealloc_leaf() { return _workstealtree.get_leaf_num(); }
  task_attrs get_task_attrs() const { return _task_attrs;}


#ifdef ADAPTIVE_PFOR_DEBUG
  const tree_type& get_tree() const { return _workstealtree; }
#endif // ADAPTIVE_PFOR_DEBUG

private:
  group_ptr  _group;
  tree_type  _workstealtree;
  f_type     _f;
  task_attrs _task_attrs;
  bool       _prealloc;

  // Disable all copying and movement.
  MARE_DELETE_METHOD(adaptive_pfor_strategy(adaptive_pfor_strategy const&));
  MARE_DELETE_METHOD(adaptive_pfor_strategy(adaptive_pfor_strategy&&));
  MARE_DELETE_METHOD(adaptive_pfor_strategy&
      operator=(adaptive_pfor_strategy const&));
  MARE_DELETE_METHOD(adaptive_pfor_strategy&
      operator=(adaptive_pfor_strategy&&));

}; // class adaptive_pfor_strategy


// Work on a worksteal tree node until stealing is detected or finish.
// This API is the evil twin brother of try_steal. If progress is modified
// by the stealer task, the owner will start looking for work from the
// stolen node instead of from the root of the tree.
// Return true if complete the range and false if detected stolen.

template<typename UnaryFn>
bool work_on(ws_node::node_type* node, UnaryFn&& fn) {

  MARE_INTERNAL_ASSERT(node != nullptr, "Unexpectedly work on null pointer");
  MARE_INTERNAL_ASSERT(node->is_unclaimed() == false,
                       "Invalid node state %zu",
                       node->get_progress());

  ws_node::size_type first = node->get_first();
  ws_node::size_type last = node->get_last();

  if (first > last) {
    node->set_completed();
    return true;
  }

  auto blk_size = node->get_tree()->get_blk_size();
  auto i = first;
  ws_node::size_type right_bound;

  // if it's root, or the first left child in pre-built tree, we must
  // finish the range [first, first + _blk_size)
  // before checking for stealing
  //
  if (first == node->get_tree()->range_start()) {
    right_bound = i + blk_size - 1;
    right_bound = right_bound > last ? last : right_bound;
    while (i <= right_bound) {
      fn(i++);
    }

    if (right_bound == last)
      return true;
  }

  // work on assigned iterations and check for stealing after
  // a batch is done.
  do {

    // process blk_size iterations in a batch
    right_bound = i + blk_size - 1;
    right_bound = right_bound > last ? last : right_bound;

    auto j = i;
    while (j <= right_bound) {
      fn(j++);
    }

    // Increase progress atomically
    auto prev = node->inc_progress(blk_size, std::memory_order_relaxed);

    // If the previous value is not what we expect, that
    // means that someone has stolen the node from us.
    if (i != prev) {
      MARE_INTERNAL_ASSERT(node->is_stolen(blk_size),
                           "Invalid node state %zu", node->get_progress());
      return false;
    }

    // inc_progress calls fetch_add, which
    // returns the previous value of progress,
    // so we have to increase i accordingly
    i = prev + blk_size;

  } while(i <= last);

  node->set_completed();
  return true;
}

template<typename Strategy>
void
stealer_task_body(Strategy& strategy, size_t task_id)
{
    MARE_INTERNAL_ASSERT(internal::current_task() != nullptr,
                         "current task pointer unexpectedly invalid");

    auto work_item = strategy.get_root();

    // tree is pre-built.
    if (strategy.is_prealloc() && task_id < strategy.get_prealloc_leaf()) {
      work_item = strategy.find_work_prebuilt(task_id);
    }
    else{
      // when task_id is 0, it always claims root
      if (task_id) {
        work_item = strategy.find_work_intree
          (strategy.get_root(), strategy.get_blk_size());
      }
    }

    bool work_complete;

    if (work_item == nullptr)
      return ;

    // If necessary, launch a new stealer task.
    if (task_id < strategy.get_max_tasks() - 1) {
      auto attrs = strategy.get_task_attrs();
      launch(strategy.get_group(), with_attrs(attrs, [&strategy, task_id] ()
             mutable { stealer_task_body<Strategy>(strategy, task_id + 1); }
          ));
    }

    do {
#ifdef ADAPTIVE_PFOR_DEBUG
        //save task id for graphviz output
        work_item->set_worker_id(task_id);
#endif //ADAPTIVE_PFOR_DEBUG
        work_complete = work_on(work_item, strategy.get_unary_fn());
        if (work_complete)
          work_item = strategy.find_work_intree
            (strategy.get_root(), strategy.get_blk_size());
        // if work not complete, meaning stealing happened during
        // work on, we search for work from current node instead of root
        else
          work_item = strategy.find_work_intree
            (work_item, strategy.get_blk_size());
     } while(work_item != nullptr);
}

// Helper functions for debugging with graphviz
#ifdef ADAPTIVE_PFOR_DEBUG
void print_node(ws_node::node_type* root, std::ofstream& f);
void print_edge(ws_node::node_type* curr, std::ofstream& f);
void print_tree(const ws_node::tree_type& tree);
#endif // ADAPTIVE_PFOR_DEBUG

} // namespace internal
} // namsapce mare

