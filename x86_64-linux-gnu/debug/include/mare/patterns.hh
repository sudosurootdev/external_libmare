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
/** @file patterns.hh */
#pragma once

// TODO:
// * fold / gather (tricky)
// * BFS (non-strict and strict = level-synchronized), randomized DFS
// * sorting
// * searching
// * use task attrs to figure out best blocking strategy, current strategy
//   is not optimal in the presence of other load
// * return group?
// * documentation

#include <algorithm>
#include <atomic>
#include <iterator>
#include <memory>

#include <mare/attr.hh>
#include <mare/range.hh>
#include <mare/runtime.hh>
#include <mare/task.hh>
#include <mare/index.hh>

#include <mare/internal/adaptivepfor.hh>
#include <mare/internal/compat.h>
#include <mare/internal/gpupatterns.hh>
#include <mare/internal/taskfactory.hh>
#include <mare/metatype/distance.hh>

namespace mare {

namespace {

template<class T>
T static_chunk_size(T count)
{
  // Inexpensive way to create fine-grained tasks; the real solution is
  // outlined below (adaptable or user-configurable chunking)
  T const GRANULARITY_MULTIPLIER = 4;
  return std::max(T(1),
                  count /
                  (GRANULARITY_MULTIPLIER *
                   static_cast<T>(internal::num_execution_contexts())));
}

template <typename NullaryFn>
void
launch_or_exec(bool not_last, group_ptr g, NullaryFn&& fn)
{
  auto attrs = create_task_attrs(mare::internal::attr::pfor);
  if (not_last) {
    launch(g, with_attrs(attrs, fn));
  } else {
    auto master = create_task(with_attrs(attrs, fn));
    internal::c_ptr(master)->execute_inline(internal::c_ptr(g));
  }
}

template<typename Strategy>
void
execute_master_task(Strategy& strategy)
{
  auto attrs = create_task_attrs(mare::internal::attr::pfor);
  auto master = create_task(mare::with_attrs(attrs, [&strategy] ()
          mutable { internal::stealer_task_body<Strategy>(strategy, 0); }
        ));
  internal::c_ptr(master)->execute_inline
    (internal::c_ptr(strategy.get_group()));
}

}; // namespace

#ifdef ONLY_FOR_DOXYGEN
/** @addtogroup gpu_patterns
    @{ */
/**
    Parallel version of <code>std::for_each</code> that runs on the GPU.

    Launches a task on the GPU with global size for the OpenCL C kernel
    as defined by <code>mare::range</code> parameter.

    @note1 This function returns only after the kernel completes execution.

    In addition, the call to this function cannot be canceled by
    canceling the group passed as argument.

    @param r Range object (1D, 2D or 3D) representing the iteration space.
    @param attrd_body Body with attributes, kernel object
                      created using <tt>mare::create_kernel</tt>
                      and actual kernel arguments.

    @par Example 1 Create a pfor_each which runs on the GPU.
    @includelineno examples/vector-add-gpu-pfor-each.cc

    @sa <tt>mare::create_ndrange_task</tt>
*/

template<size_t DIMS, typename Body>
void pfor_each(mare::range<DIMS>& r,
               body_with_attrs_gpu<
                Body,KernelPtr, Kargs...>&& attrd_body);

/** @} */ /* end_addtogroup gpu_patterns */
#endif


/** @addtogroup patterns_doc
    @{ */
/**
    Parallel version of <code>std::for_each</code> (asynchronous).

    Applies function object <code>fn</code> in parallel to every iterator in
    the range [first, last).

    @note1 In contrast to <code>std::for_each</code> and
    <code>ptransform</code>, the iterator
    is passed to the function, instead of the element.

    It is permissible to modify the elements of the range from <code>fn</code>,
    assuming that <code>InputIterator</code> is a mutable iterator.

    @note1 This function does NOT wait for all function
    applications to finish.  Callers must wait on <code>g</code>
    (see <code>wait_for</code>), if this is desired.

    @par Complexity
    Exactly <code>last-first</code> applications of <code>fn</code>.

    @sa ptransform(group_ptr, InputIterator, InputIterator, UnaryFn)
    @sa pfor_each(group_ptr, InputIterator, InputIterator, UnaryFn&&)

    @param g     All MARE tasks created are added to this group.
    @param first Start of the range to which to apply <code>fn</code>.
    @param last  End of the range to which to apply <code>fn</code>.
    @param fn    Unary function object to be applied.
*/
template <class InputIterator, typename UnaryFn>
void pfor_each_async(group_ptr g, InputIterator first, InputIterator last,
                     UnaryFn fn) {
  typedef typename internal::distance_helper<InputIterator>::
    _result_type working_type;

  if (first >= last)
    return;

  working_type const dist = internal::distance(first,last);
  // blksz could be picked better, and should perhaps also be under
  // user control.  E.g., if fn is expensive, we may want smaller
  // blocks; Splitting into blocks should possibly be moved elsewhere.
  // Also, alignment, blk_size, etc., may be dependent on fn.
  working_type blksz = static_chunk_size<working_type>(dist);
  MARE_INTERNAL_ASSERT(blksz > 0, "block size should be greater than zero");
  InputIterator lb = first;
  while (lb < last) {
    auto safeofs = std::min(working_type(internal::distance(lb,last)), blksz);
    InputIterator rb = lb + safeofs;

    launch(g, [=] {
        InputIterator it = lb;
        while (it < rb)
          fn(it++);
      });
    lb = rb;
  }
}

/** @addtogroup patterns_doc
    @{ */

/**
    \brief Parallel version of <code>std::for_each</code> (asynchronous).

    \see pfor_each(mare::range<DIMS>&, body_with_attrs_gpu<
                   Body, KernelPtr, Kargs...>&&)
    \see pfor_each_async(group_ptr, InputIterator, InputIterator, UnaryFn)

    @param g     All MARE tasks created are added to this group.
    @param r     Range object (1D, 2D or 3D) representing the iteration space.
    @param fn    the unary function object to be applied
*/

template<size_t DIMS, typename UnaryFn>
void pfor_each_async(group_ptr g, const mare::range<DIMS>& r, UnaryFn fn)
{
  //range can have offsets, but bounds are always zero based,
  //mare::range::linear_to_index will take care of adding the offsets
  //if any in range.
  pfor_each_async(g, size_t(0), r.size(), [=] (size_t i) {
    index<DIMS> idx = r.linear_to_index(i);
    fn(idx);
  });
}

/** @} */ /* end_addtogroup patterns_doc*/

/** @cond HIDDEN */

template <class InputIterator, typename UnaryFn>
void
pfor_each_iterator(group_ptr group, InputIterator first, InputIterator last,
               UnaryFn&& fn) {

  if (first >= last)
    return;
  auto g_internal = create_group("pfor_each/4");
  auto g = g_internal;
  if (group)
    g = g & group;

  pfor_each_async(g, first, last, std::forward<UnaryFn>(fn));
  wait_for(g_internal);
}

template <size_t DIMS, typename UnaryFn>
void pfor_each_range(group_ptr group, const mare::range<DIMS>& r, UnaryFn fn)
{
  auto g_internal = create_group("pfor_each/4");
  auto g = g_internal;
  if (group)
    g = g & group;

  pfor_each_async(g, r, std::forward<UnaryFn>(fn));

  wait_for(g_internal);
}

/**
    Parallel version of <code>std::for_each</code>.

    Applies function object <code>fn</code> in parallel to every iterator in
    the range [first, last).

    @note1 This function returns only after <code>fn</code> has been applied
    to the whole iteration range.

    This function currently only applies to [first, last) with type
    size_t instead of the more general InputIterator. It is implemented
    based on a work steal tree data structure. The API has not been
    finalized yet.
*/

template<typename Body>
void
pfor_each_sizet(group_ptr group, size_t first, size_t last, Body&& body) {

  if (first >= last)
    return ;

  // Wrap the body so that we get a nice interface to it
  typedef mare::internal::body_wrapper<Body> pfor_body_wrapper;
  pfor_body_wrapper wrapper(std::forward<Body>(body));

  // Let's make sure that the function passed is unary
  MARE_INTERNAL_ASSERT(pfor_body_wrapper::get_arity() == 1,
                       "invalid number of arguments in pfor body");

  // Let's get the body attributes, and add pfor to them
  auto body_attrs = wrapper.get_attrs();
  body_attrs = mare::add_attr(body_attrs, mare::internal::attr::pfor);

  // We get a reference to the wrapper
  auto& fn = wrapper.get_fn();
  typedef typename std::remove_reference<decltype(fn)>::type UnaryFn;

  // blk_size defines the minimal granuality of iterations to advance.
  // TODO: We have to figure out how to pass blk_size to API.
  const size_t blk_size = 1;

  // We first check whether this is a nested pfor or not.
  // It is, we treat it differently depending on whether
  // the user passed a group pointer to it or not:
  //   - if she didn't, then we execute the pfor as a regular for.
  //   - if she did, we create a task that executes it as a regular
  //     for, but we execute it inline.
  //
  auto t = internal::current_task();
  if (t && t->is_pfor()) {

    if (group == nullptr) {
      for (size_t i = first; i < last; ++i)
        fn(i);
      return ;
    }

    auto inlined = mare::create_task(mare::with_attrs(body_attrs,
        [first, last, &fn] {
          for (size_t i = first; i < last; ++i) {
            fn(i);
          }
      }));
    internal::c_ptr(inlined)->execute_inline(internal::c_ptr(group));
    return;
  };

  // Create the group that will take care of the parallel for
  auto g_internal = create_group();
  auto g = g_internal;
  if(group)
    g = g & group;

  typedef internal::adaptive_pfor_strategy<UnaryFn> strategy_type;

  // adaptive_pfor is in fact the storage with a work steal tree,
  // the group pointer, and the lambda.
  strategy_type adaptive_pfor(g, first, last - 1,
                              std::forward<UnaryFn>(fn),
                              body_attrs,
                              blk_size);

  size_t max_tasks = adaptive_pfor.get_max_tasks();
  if (max_tasks > 1 && max_tasks < (last - first))
    adaptive_pfor.static_split(max_tasks);

  // We are the ones that found the parallel for, so we start
  // stealing from the pfor tree inline. If the pfor is short
  // enough, we might not even spawn many new tasks.
  execute_master_task<strategy_type>(adaptive_pfor);

  spin_wait_for(g);

#ifdef ADAPTIVE_PFOR_DEBUG
  print_tree(adaptive_pfor.get_tree());
#endif // ADAPTIVE_PFOR_DEBUG
}

template<class InputIterator, typename UnaryFn, typename T>
struct pfor_each_dispatcher;

//GPU version of pfor.
template<typename UnaryFn>
struct pfor_each_dispatcher<size_t, UnaryFn, std::true_type>
{
  //Routes it to size_t verion on gpu
  static void dispatch(group_ptr group, size_t first, size_t last,
                       UnaryFn&& fn) {
    pfor_each_gpu(group, first, last, std::forward<UnaryFn>(fn));
  }

  //Routes it to mare::range<DIMS> version on gpu.
  template<size_t DIMS>
  static void dispatch(group_ptr group, const mare::range<DIMS>& r,
                       UnaryFn&& fn) {
    pfor_each_gpu(group, r, std::forward<UnaryFn>(fn));
  }
};

// Dispatch pfor_each call for type non_sizet inputIterators
template<class InputIterator, typename UnaryFn, typename T>
struct input_iterator_dispatcher;

// For integral types
template<class InputIterator, typename UnaryFn>
struct input_iterator_dispatcher<InputIterator, UnaryFn, std::true_type>
{
  static void iterator_dispatch(group_ptr group, InputIterator first,
                                InputIterator last, UnaryFn&& fn) {
    auto fn_transform = [fn, first](size_t i){
      fn(i + first);
    };
    pfor_each_sizet(group, size_t(0), size_t(last - first),
                    std::forward<decltype(fn_transform)>(fn_transform));
  }
};

// For all other non-integral types
template<class InputIterator, typename UnaryFn>
struct input_iterator_dispatcher<InputIterator, UnaryFn, std::false_type>
{
  static void iterator_dispatch(group_ptr group, InputIterator first,
                                InputIterator last, UnaryFn&& fn) {
    pfor_each_iterator(group, first, last, std::forward<UnaryFn>(fn));
  }
};

//CPU versions
template<class InputIterator, typename UnaryFn>
struct pfor_each_dispatcher<InputIterator, UnaryFn, std::false_type>
{
  //Routes it to iterator version on cpu
  static void dispatch(group_ptr group, InputIterator first,
                       InputIterator last, UnaryFn&& fn) {
    input_iterator_dispatcher<InputIterator, UnaryFn,
      typename std::is_integral<InputIterator>::type>::
      iterator_dispatch(group, first, last, std::forward<UnaryFn>(fn));
  }
};

template<typename UnaryFn>
struct pfor_each_dispatcher<size_t, UnaryFn, std::false_type>
{
  //Routes it to size_t version on cpu
  static void dispatch(group_ptr group, size_t first, size_t last,
                       UnaryFn&& fn) {
    pfor_each_sizet(group, first, last, std::forward<UnaryFn>(fn));
  }

  template<size_t DIMS>
  static void dispatch(group_ptr group, const mare::range<DIMS>& r,
                       UnaryFn&& fn) {
   pfor_each_range(group, r, std::forward<UnaryFn>(fn));
  }
};

/** @endcond */

/**
    Parallel version of <code>std::for_each</code>.

    Applies function object <code>fn</code> in parallel to every iterator in
    the range [first, last).

        @note1 In contrast to <code>std::for_each</code> and ptransform, the
        iterator is passed to the function, instead of the element.

    It is permissible to modify the elements of the range from <code>fn</code>,
    provided that <code>InputIterator</code> is a mutable iterator.

    @note1 This function returns only after <code>fn</code> has been applied
    to the whole iteration range.

    In addition, the call to this function can be canceled by
    canceling the group passed as argument. However, in the presence
    of cancelation it is undefined to which extent the iteration
    space will have been processed.

    @note1 The usual rules for cancelation apply, i.e.,
    within <code>fn</code> the cancelation must be acknowledged using
    <code>abort_on_cancel</code>.

    @par Complexity
    Exactly <code>std::distance(first,last)</code> applications of
    <code>fn</code>.

    @sa ptransform(group_ptr, InputIterator, InputIterator, UnaryFn) \n
    @sa pfor_each_async(group_ptr, InputIterator, InputIterator, UnaryFn) \n
    @sa abort_on_cancel()

    @par Examples
    @code
    group_ptr g = create_group("g");
    [...]
    // Parallel for-loop using indices
    pfor_each(g, size_t(0), vin.size(),
              [=,&vin,&vout] (size_t i) {
                  while (!finished_lengthy_computation()) {
                     abort_on_cancel();
                     process(i);
              });
    [...]
    // elsewhere:
    cancel(g);
    @endcode

    @param group All MARE tasks created are added to this group.
    @param first Start of the range to which to apply <code>fn</code>.
    @param last  End of the range to which to apply <code>fn</code>.
    @param fn    Unary function object to be applied.
*/

template <class InputIterator, typename UnaryFn>
void
pfor_each(group_ptr group, InputIterator first, InputIterator last,
          UnaryFn&& fn)
{
  //pfor_each dispatcher routes it to different versions.
  pfor_each_dispatcher<InputIterator, UnaryFn,
    typename std::is_base_of<body_with_attrs_base_gpu, UnaryFn>::type>::
    dispatch(group, first, last, std::forward<UnaryFn>(fn));
}

/**
    \brief Parallel version of std::for_each

    \see pfor_each(group_ptr, InputIterator, InputIterator, UnaryFn&&)
    \see ptransform(group_ptr, InputIterator, InputIterator, UnaryFn)
    \see pfor_each_async(group_ptr, InputIterator, InputIterator, UnaryFn)

    \par Examples
    \code
    // Parallel for-loop using indices
    vector<size_t> vout(vin.size());
    // vout[i] := 2*vin[i]
    pfor_each(size_t(0), vin.size(),
              [=,&vin,&vout] (size_t i) {
                  vout[i] = 2*vin[i];
              });
    \endcode

    \param first start of the range to which to apply 'fn'
    \param last end of the range to which to apply 'fn'
    \param fn the unary function object to be applied
*/

template <class InputIterator, typename UnaryFn>
void pfor_each(InputIterator first, InputIterator last, UnaryFn&& fn)
{
  pfor_each(nullptr, first, last, std::forward<UnaryFn>(fn));
}

/** @addtogroup patterns_doc
    @{ */

/**
    \brief Parallel version of <code>std::for_each</code>

    \see pfor_each(mare::range<DIMS>&, body_with_attrs_gpu<
                   Body, KernelPtr, Kargs...>&&)
    \see pfor_each(group_ptr, InputIterator, InputIterator, UnaryFn&&)

    @param g     All MARE tasks created are added to this group.
    @param r     Range object (1D, 2D or 3D) representing the iteration space.
    @param fn    the unary function object to be applied
*/

template<size_t DIMS, typename UnaryFn>
void pfor_each(group_ptr group, const mare::range<DIMS>& r, UnaryFn&& fn)
{
  //pfor_each dispatcher routes it to different mare::range<DIMS> version.
  pfor_each_dispatcher<size_t, UnaryFn,
    typename std::is_base_of<body_with_attrs_base_gpu, UnaryFn>::type>::
    dispatch(group, r, std::forward<UnaryFn>(fn));
}

/**
    \brief Parallel version of <code>std::for_each</code>.

    \see pfor_each(mare::range<DIMS>&, body_with_attrs_gpu<
                   Body, KernelPtr, Kargs...>&&)
    \see pfor_each(InputIterator, InputIterator, UnaryFn)

    @param r     Range object (1D, 2D or 3D) representing the iteration space.
    @param fn    the unary function object to be applied
*/

template <size_t DIMS, typename UnaryFn>
void pfor_each(const mare::range<DIMS>& r, UnaryFn&& fn)
{
  pfor_each(nullptr, r, std::forward<UnaryFn>(fn));
}

/** @} */ /* end_addtogroup patterns_doc */

/**
    Parallel version of <code>std::transform</code>.

    Applies function object <code>fn</code> in parallel to every dereferenced
    iterator in the range [first, last) and stores the return value in
    another range, starting at <code>d_first</code>.

    @note1 This function returns only after <code>fn</code> has been applied
    to the whole iteration range.

    In addition, the call to this function can be canceled by
    canceling the group passed as argument. However, in the presence
    of cancelation it is undefined to which extent the iteration
    space will have been processed.

    @note1 The usual rules for cancelation apply, i.e.,
    within <code>fn</code> the cancelation must be acknowledged using
    <code>abort_on_cancel</code>.

    @par Complexity
    Exactly <code>std::distance(first,last)</code> applications of
    <code>fn</code>.

    @par See Also
    pfor_each

    @par Examples
    @code
    // arr[i] == 2*vin[N-i]
    size_t arr[vin.size()];
    ptransform(group, begin(vin), end(vin), arr,
               [=] (size_t const& i) {
                 return 2*i;
               });
    @endcode

    @param group   All MARE tasks created are added to this group.
    @param first   Start of the range to which to apply <code>fn</code>.
    @param last    End of the range to which to apply <code>fn</code>.
    @param d_first Start of the destination range.
    @param fn      Unary function object to be applied.
*/
template <typename InputIterator, typename OutputIterator, typename UnaryFn>
void ptransform(group_ptr group, InputIterator first, InputIterator last,
                OutputIterator d_first, UnaryFn fn) {
  // In principle, pfor_each() is sufficient as a building block for all
  // ptransform() variants, but a direct implementation needs less
  // iterator frobbing.
  if (first >= last)
    return;
  auto g_internal = create_group("ptransform/4");
  auto g = g_internal;
  if (group)
    g = g & group;

  size_t const dist = std::distance(first,last);
  auto blksz = static_chunk_size(dist);
  MARE_INTERNAL_ASSERT(blksz > 0, "block size should be greater than zero");
  auto lb = first;
  auto d_lb = d_first;
  while (lb < last) {
    auto safeofs = std::min(size_t(std::distance(lb,last)), blksz);
    InputIterator rb = lb + safeofs;

    launch_or_exec(rb != last, g, [=] {
        InputIterator it = lb;
        OutputIterator d_it = d_lb;
        while (it < rb)
          *d_it++ = fn(*it++);
      });
    lb = rb;
    d_lb += safeofs;
  }

  spin_wait_for(g_internal);
}

/**
    Parallel version of <code>std::transform</code>.

    @sa ptransform(group_ptr, InputIterator, InputIterator, UnaryFn)
    @sa pfor_each(group_ptr, InputIterator, InputIterator, UnaryFn&&)

    @par Examples
    @code
    // arr[i] == 2*vin[N-i]
    size_t arr[vin.size()];
    ptransform(begin(vin), end(vin), arr,
               [=] (size_t const& i) {
                 return 2*i;
               });
    @endcode

    @param first   Start of the range to which to apply <code>fn</code>.
    @param last    End of the range to which to apply <code>fn</code>.
    @param d_first Start of the destination range.
    @param fn      Unary function object to be applied.
*/
template <typename InputIterator, typename OutputIterator, typename UnaryFn>
void ptransform(InputIterator first, InputIterator last,
                OutputIterator d_first, UnaryFn&& fn) {
  ptransform(nullptr,
             std::forward<InputIterator>(first),
             std::forward<InputIterator>(last),
             std::forward<OutputIterator>(d_first),
             std::forward<UnaryFn>(fn));
}

/**
    Parallel version of <code>std::transform</code>.

    Applies function object <code>fn</code> in parallel to every pair of
    dereferenced destination iterators in the range [first1, last1)
    and [first2,...), and stores the return value in another range,
    starting at <code>d_first</code>.

    @note1 This function returns only after <code>fn</code> has been applied
    to the whole iteration range.

    In addition, the call to this function can be canceled by
    canceling the group passed as argument. However, in the presence
    of cancelation it is undefined to which extent the iteration
    space will have been processed.

    @note1 The usual rules for cancelation apply, i.e.,
    within <code>fn</code> the cancelation must be acknowledged using
    <code>abort_on_cancel</code>.

    @par Complexity
    Exactly <code>std::distance(first1,last1)</code> applications of
    <code>fn</code>.

    @sa pfor_each(group_ptr, InputIterator, InputIterator, UnaryFn&&)

    @par Examples
    @code
    // vout[i] == vin[i] + vin[i+1]
    vector<size_t> vout(vin.size()-1);
    ptransform(group,
               begin(vin), begin(vin)+vout.size(),
               begin(vin)+1,
               begin(vout),
               [=] (size_t const& i, size_t const& j) {
                 return i+j;
               });
    @endcode

    @param group   All MARE tasks created are added to this group.
    @param first1  Start of the range to which to apply <code>fn</code>.
    @param last1   End of the range to which to apply <code>fn</code>.
    @param first2  Start of the second range to which to apply <code>fn</code>.
    @param d_first Start of the destination range.
    @param fn      Binary function object to be applied.
*/
template <typename InputIterator, typename OutputIterator, typename BinaryFn>
void ptransform(group_ptr group,
                InputIterator first1, InputIterator last1,
                InputIterator first2,
                OutputIterator d_first, BinaryFn fn) {
  if (first1 >= last1)
    return;
  auto g_internal = create_group("ptransform/5");
  auto g = g_internal;
  if (group)
    g = g & group;

  size_t const dist = std::distance(first1,last1);
  auto blksz = static_chunk_size(dist);
  MARE_INTERNAL_ASSERT(blksz > 0, "block size should be greater than zero");
  auto lb = first1;
  auto lb2 = first2;
  auto d_lb = d_first;
  while (lb < last1) {
    auto safeofs = std::min(size_t(std::distance(lb,last1)), blksz);
    InputIterator rb = lb + safeofs;

    launch_or_exec(rb != last1, g, [=] {
        InputIterator it = lb;
        InputIterator it2 = lb2;
        OutputIterator d_it = d_lb;
        while (it < rb)
          *d_it++ = fn(*it++, *it2++);
      });
    lb = rb;
    lb2 += safeofs;
    d_lb += safeofs;
  }

  spin_wait_for(g_internal);
}

/**
    Parallel version of <code>std::transform</code>.

    Applies function object <code>fn</code> in parallel to every pair of
    dereferenced destination iterators in the range [first1, last1)
    and [first2,...), and stores the return value in another range,
    starting at <code>d_first</code>.

    @note1 This function returns only after <code>fn</code> has been applied
    to the whole iteration range.

    @par Complexity
    Exactly <code>std::distance(first1,last1)</code> applications of
    <code>fn</code>.

    @sa ptransform(group_ptr, InputIterator, InputIterator, UnaryFn)
    @sa pfor_each(group_ptr, InputIterator, InputIterator, UnaryFn&&)

    @par Examples
    @code
    // vout[i] == vin[i] + vin[i+1]
    vector<size_t> vout(vin.size()-1);
    ptransform(begin(vin), begin(vin)+vout.size(),
               begin(vin)+1,
               begin(vout),
               [=] (size_t const& i, size_t const& j) {
                 return i+j;
               });
    @endcode

    @param first1  Start of the range to which to apply <code>fn</code>.
    @param last1   End of the range to which to apply <code>fn</code>.
    @param first2  Start of the second range to which to apply <code>fn</code>.
    @param d_first Start of the destination range.
    @param fn      Binary function object to be applied.
*/
template <typename InputIterator, typename OutputIterator, typename BinaryFn>
void ptransform(InputIterator first1, InputIterator last1,
                InputIterator first2,
                OutputIterator d_first, BinaryFn&& fn) {
  ptransform(nullptr, first1, last1, first2, d_first,
             std::forward<BinaryFn>(fn));
}

/**
    Parallel version of <code>std::transform</code>.

    Applies function object <code>fn</code> in parallel to every dereferenced
    iterator in the range [first, last).

    @note1 In contrast to pfor_each, the dereferenced iterator is passed
    to the function.

    It is permissible to modify the elements of the range from <code>fn</code>,
    assuming that <code>InputIterator</code> is a mutable iterator.

    @note1 This function returns only after <code>fn</code> has been applied
    to the whole iteration range.

    In addition, the call to this function can be canceled by
    canceling the group passed as argument. However, in the presence
    of cancelation it is undefined to which extent the iteration
    space will have been processed.

    @note1 The usual rules for cancelation apply, i.e.,
    within <code>fn</code> the cancelation must be acknowledged using
    <code>abort_on_cancel</code>.

    @par Complexity
    Exactly <code>std::distance(first,last)</code> applications of
    <code>fn</code>.

    @sa pfor_each(group_ptr, InputIterator, InputIterator, UnaryFn&&)

    @par Examples
    @code
    // Parallel for-loop using indices
    vector<size_t> vout(vin.size());
    // vout[i] := 2*vin[i]
    pfor_each(group,
              size_t(0), vin.size(),
              [=,&vin,&vout] (size_t i) {
                  vout[i] = 2*vin[i];
              });
    @endcode

    @par
    @code
    // Parallel for-loop using iterators
    vector<size_t> vout(vin.size());
    // vout[i] := 2*vin[i]
    pfor_each(group,
              begin(vin), end(vin),
              [&vin,&vout] (vector<size_t>::const_iterator it) {
                  vout[it - begin(vin)] = 2 * *it;
              });
    @endcode

    @param group All MARE tasks created are added to this group.
    @param first Start of the range to which to apply <code>fn</code>.
    @param last  End of the range to which to apply <code>fn</code>.
    @param fn    Unary function object to be applied.
*/
template <typename InputIterator, typename UnaryFn>
void ptransform(group_ptr group, InputIterator first, InputIterator last,
                UnaryFn fn) {
  pfor_each(group, first, last, [fn] (InputIterator it) { fn(*it); });
}

/**
    Parallel version of <code>std::transform</code>.

    @sa ptransform(group_ptr, InputIterator, InputIterator, UnaryFn)
    @sa pfor_each(group_ptr, InputIterator, InputIterator, UnaryFn&&)

    @par Examples
    @code
    // Parallel for-loop using indices
    vector<size_t> vout(vin.size());
    // vout[i] := 2*vin[i]
    pfor_each(size_t(0), vin.size(),
              [=,&vin,&vout] (size_t i) {
                  vout[i] = 2*vin[i];
              });
    @endcode

    @par
    @code
    // Parallel for-loop using iterators
    vector<size_t> vout(vin.size());
    // vout[i] := 2*vin[i]
    pfor_each(begin(vin), end(vin),
              [&vin,&vout] (vector<size_t>::const_iterator it) {
                  vout[it - begin(vin)] = 2 * *it;
              });
    @endcode

    @param first Start of the range to which to apply <code>fn</code>.
    @param last  End of the range to which to apply <code>fn</code>.
    @param fn    Unary function object to be applied.
*/
template <typename InputIterator, typename UnaryFn>
void ptransform(InputIterator first, InputIterator last, UnaryFn&& fn) {
  ptransform(nullptr, first, last, std::forward<UnaryFn>(fn));
}
/** @} */ /* end_addtogroup patterns_doc */

// Sklansky-style parallel inclusive scan
namespace internal {

template <typename InputIterator, typename BinaryFn>
void pscan_inclusive_rec(group_ptr g, InputIterator first, InputIterator last,
                         BinaryFn fn) {
  size_t dist = std::distance(first,last);
  if (dist < 2)
    return;

  auto pivot = first + dist/2;
  pscan_inclusive_rec(g, first, pivot, fn);
  pscan_inclusive_rec(g, pivot, last, fn);

  spin_wait_for(g);

  launch(g, [=] {
      // like *(pivot-1), but with fwd iters only
      auto const& lhs = *(first + (dist/2-1));
      pfor_each_async(g, pivot, last, [=] (InputIterator it) {
          *it = fn(lhs, *it);
        });
    });
};

}; //namespace internal

/** @addtogroup patterns_doc
@{ */
/**
    Sklansky-style parallel inclusive scan.

    Performs an in-place parallel prefix computation using the
    function object <code>fn</code> for the range [first, last).

    It is not permissible for <code>fn</code> to modify the elements of the
    range. Also, <code>fn</code> should be associative, because the order of
    applications is not fixed.

    @note1 This function returns only after <code>fn</code> has been applied
    to the whole iteration range.

    In addition, the call to this function can be canceled by
    canceling the group passed as argument. However, in the presence
    of cancelation it is undefined to which extent the iteration
    space will have been processed.

    @note1 The usual rules for cancelation apply, i.e.,
    within <code>fn</code> the cancelation must be acknowledged using
    <code>abort_on_cancel</code>.

    @par Examples
    @code
    // After: v' = { v[0], v[0] x v[1], v[0] x v[1] x v[2], ... }
    pscan_inclusive(group,
                    begin(v), end(v),
                    [] (size_t const& i, size_t const& j) {
                        return i + j;
                    });
    @endcode

    @param group All MARE tasks created are added to this group.
    @param first Start of the range to which to apply <code>fn</code>.
    @param last  End of the range to which to apply <code>fn</code>.
    @param fn    Binary function object to be applied.
*/
template <typename InputIterator, typename BinaryFn>
void pscan_inclusive(group_ptr group,
                     InputIterator first, InputIterator last, BinaryFn fn) {
  auto g_internal = create_group("pscan_inclusive");
  auto g = g_internal;
  if (group)
    g = g & group;

  internal::pscan_inclusive_rec(g, first, last, fn);
  spin_wait_for(g_internal);
}

/**
    Sklansky-style parallel inclusive scan.

    @sa pscan_inclusive(group_ptr, InputIterator, InputIterator, BinaryFn)

    @par Examples
    @code
    // After: v' = { v[0], v[0] x v[1], v[0] x v[1] x v[2], ... }
    pscan_inclusive(begin(v), end(v),
                    [] (size_t const& i, size_t const& j) {
                        return i + j;
                    });
    @endcode

    @param first Start of the range to which to apply <code>fn</code>.
    @param last  End of the range to which to apply <code>fn</code>.
    @param fn    Binary function object to be applied.
*/
template <typename InputIterator, typename BinaryFn>
void pscan_inclusive(InputIterator first, InputIterator last, BinaryFn&& fn) {
  pscan_inclusive(nullptr, first, last, std::forward<BinaryFn>(fn));
}

/** @} */ /* end_addtogroup patterns_doc */
}; // namespace mare
