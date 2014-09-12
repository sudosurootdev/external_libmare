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

#include <mare/attr.hh>
#include <mare/attrobjs.hh>
#include <mare/gpukernel.hh>

#include <mare/internal/gputask.hh>
#include <mare/internal/group.hh>
#include <mare/internal/macros.hh>
#include <mare/internal/maretask.hh>
#include <mare/internal/task.hh>
#include <mare/internal/templatemagic.hh>

namespace mare {

namespace internal {

// We disable -Weffc++ for this file because the template metaprogramming here
// involves a great deal of subclassing of classes with pointer members. This
// causes -Weffc++ to emit huge numbers of warnings requesting that we write
// copy constructors and assignment operators; however, in these cases the
// defaults are sufficient, and manually defaulting all of the implementations
// will harm code readability greatly for no benefit.
MARE_GCC_IGNORE_BEGIN("-Weffc++");

template <typename Body>
struct body_wrapper_base;

/// Matches functions, lambdas, and member functions used
/// to create tasks.
template<typename Fn>
struct body_wrapper_base
{
  typedef function_traits<Fn> traits;
  typedef typename has_cancel_notify<Fn>::type has_cancel_notify_type;

  /// Returns function, lambda or member function.
  /// \return
  ///  Fn -- Function, lambda or member function used as body.
  MARE_CONSTEXPR  Fn& get_fn() const {
    return _fn;
  }

  /// Returns default attributes because the user did not
  /// specify attributes for Fn.
  /// \return
  ///    task_attrs Default attributes for body.
  MARE_CONSTEXPR  task_attrs get_attrs() const {
    return create_task_attrs();
  }

  /// Constructor
  /// \param fn Function
  MARE_CONSTEXPR  body_wrapper_base(Fn&& fn):_fn(fn){}

  /// Creates tasks from body.
  /// \return
  /// task*  Pointer to the newly created task.
  task* create_task() {
    return create_task(nullptr, create_task_attrs(attr::none));
  }

  /// Creates tasks from body.
  /// \param g Group the task should be added to.
  /// \param attrs Tasks attributes.
  /// \return
  /// task*  Pointer to the newly created task.
  task* create_task(group *g, task_attrs attrs) {
    return new mare_task<Fn>(std::forward<Fn>(_fn), g, attrs);
  }

  /// Creates tasks from fn.
  /// \param fn Code that the task will execute.
  /// \return
  /// task*  Pointer to the newly created task.
  static
  task* create_task(Fn&& fn) {
    return create_task(std::forward<Fn>(fn), nullptr);
  }

  /// Creates tasks from fn and adds it to group.
  /// \param fn Code that the task will execute.
  /// \param g Group the task should be added to.
  /// \return
  /// task*  Pointer to the newly created task.
  static
  task* create_task(Fn&& fn, group *g) {
    return new mare_task<Fn>(std::forward<Fn>(fn), g,
                             create_task_attrs(attr::none));
  }

private:
  Fn& _fn;
};

/// Matches functions, lambdas, and member functions used
/// to create tasks that are wrapped in body_with_attrs.
template<typename Fn>
struct body_wrapper_base< body_with_attrs<Fn> >
{

  typedef function_traits<Fn> traits;
  typedef typename has_cancel_notify<Fn>::type has_cancel_notify_type;

  /// Returns function, lambda or member function that was wrapped in
  /// body_with_attrs.
  /// \return
  ///  Fn -- Function, lambda or member function used as body.
  MARE_CONSTEXPR  Fn& get_fn() const {
    return _b.get_body();
  }

  /// Returns attributes from body_with_attrs.
  /// \return
  ///  task_attrs Attributes from body_with_attrs.
  MARE_CONSTEXPR  task_attrs get_attrs() const {
    return _b.get_attrs();
  }

  /// Constructor
  /// \param b body_with_attrs wrapper
  MARE_CONSTEXPR  body_wrapper_base(body_with_attrs<Fn> &&b):_b(b){}

  /// Creates tasks from body, using the attributes
  /// in with_attrs.
  /// \return
  /// task*  Pointer to the newly created task.
  task* create_task() {
    return create_task(nullptr, get_attrs());
  }

  /// Creates tasks from body.
  /// \param g Group the task should be added to.
  /// \param attrs Tasks attributes.
  /// \return
  /// task*  Pointer to the newly created task.
  task* create_task(group *g, task_attrs attrs) {
    MARE_API_ASSERT(!has_attr(attrs, mare::attr::blocking) ||
                    (has_cancel_notify_type::value),
                    "A blocking task requires cancel_notify support.");
    return new mare_task<Fn>(std::forward<Fn>(get_fn()), g, attrs);
  }

  /// Creates tasks from body_with_attrs.
  /// \param attr_body Code and attributes for the task.
  /// \return
  /// task*  Pointer to the newly created task.
  static
  task* create_task(body_with_attrs<Fn>&& attrd_body) {
    return create_task(std::forward<body_with_attrs<Fn>>(attrd_body), nullptr);
  }

  /// Creates tasks from fn and adds it to group.
  /// \param fn Code and atrributes for the new task.
  /// \param g Group the task should be added to.
  /// \return
  /// task*  Pointer to the newly created task.
  static
  task* create_task(body_with_attrs<Fn>&& attrd_body, group *g) {
    auto attrs = attrd_body.get_attrs();
    MARE_API_ASSERT(!has_attr(attrs, mare::attr::blocking) ||
                    (has_cancel_notify_type::value),
                    "A blocking task requires cancel_notify support.");
    return new mare_task<Fn>(std::forward<Fn>(attrd_body.get_body()),
                             g, attrs);
  }

private:
  body_with_attrs<Fn>& _b;
};

#ifdef HAVE_OPENCL
/// Template specialization for gpu task.
template<typename Fn, typename Kernel, typename ...Args>
struct body_wrapper_base< body_with_attrs_gpu<Fn, Kernel, Args...> >
{
  typedef function_traits<Fn> traits;
  typedef typename body_with_attrs_gpu<Fn, Kernel, Args...>::kernel_parameters
                   kparams;
  typedef typename body_with_attrs_gpu<Fn, Kernel, Args...>::kernel_arguments
                   kargs;

  /// Returns function, lambda or member function that was wrapped in
  /// body_with_attrs.
  /// \return
  ///  Fn -- Function, lambda or member function used as body.
  MARE_CONSTEXPR Fn& get_fn() const {
    return _b.get_body();
  }

  /// Returns attributes from body_with_attrs.
  /// \return
  ///  task_attrs Attributes from body_with_attrs.
  MARE_CONSTEXPR task_attrs get_attrs() const {
    return _b.get_attrs();
  }


  /// Constructor
  /// \param b body_with_attrs wrapper
  MARE_CONSTEXPR
  body_wrapper_base(const body_with_attrs_gpu<Fn, Kernel, Args...> &&b):_b(b){}

  /// Creates tasks from fn and adds it to group.
  /// \param fn Code and atrributes for the new task.
  /// \param g Group the task should be added to.
  /// \return
  /// task*  Pointer to the newly created task.
  template<size_t DIMS>
  static
  task* create_task(device_ptr const& device,
                    const ::mare::range<DIMS>& r,
                    body_with_attrs_gpu<Fn, Kernel, Args...>&& attrd_body) {
    auto attrs = attrd_body.get_attrs();
    return new gputask<DIMS, Fn, kparams, kargs>(
                                           device,
                                           r,
                                           attrd_body.get_body(),
                                           attrs,
                                           attrd_body.get_gpu_kernel(),
                                           attrd_body.get_kernel_args());
  }

private:
  const body_with_attrs_gpu<Fn, Kernel, Args...>& _b;
};
#endif

/// Matches functions, lambdas, and member functions used
/// to create tasks that are wrapped in body_with_attrs that
/// include a cancel handler.
template<typename Fn, typename H>
struct body_wrapper_base< body_with_attrs<Fn, H> >
{

  typedef function_traits<Fn> traits;
  typedef function_traits<H> cancel_handler_traits;
  typedef std::true_type has_cancel_notify;

  /// Returns function, lambda or member function that was wrapped in
  /// body_with_attrs.
  /// \return
  ///  Fn -- Function, lambda or member function used as body.
  MARE_CONSTEXPR  Fn& get_fn() const{
    return _b.get_body();
  }

  /// Returns attributes from body_with_attrs.
  /// \return
  ///  task_attrs Attributes from body_with_attrs.
  MARE_CONSTEXPR  task_attrs get_attrs() const {
    return _b.get_attrs();
  }

  /// Constructor
  /// \param b body_with_attrs wrapper
  MARE_CONSTEXPR  body_wrapper_base(body_with_attrs<Fn, H> &&b):
    _b(b)
  {}

  /// Creates tasks from body, using the attributes
  /// in with_attrs.
  /// \return
  /// task*  Pointer to the newly created task.
  task* create_task() {
    return create_task(nullptr, get_attrs());
  }

  /// Creates tasks from body.
  /// \param g Group the task should be added to.
  /// \param attrs Tasks attributes.
  /// \return
  /// task*  Pointer to the newly created task.
  task* create_task(group *g, task_attrs attrs) {
    return new
      mare_task_with_notify<Fn, H>(std::forward<Fn>(get_fn()),
                                   std::forward<H>(_b.get_cancel_handler()),
                                   g, attrs);
  }

  /// Creates tasks from body_with_attrs.
  /// \param attr_body Code and attributes for the task.
  /// \return
  /// task*  Pointer to the newly created task.
  static
  task* create_task(body_with_attrs<Fn,H>&& attrd_body) {
    return create_task(std::forward<body_with_attrs<Fn,H>>(attrd_body),
                       nullptr);
  }

  /// Creates tasks from fn and adds it to group.
  /// \param fn Code and atrributes for the new task.
  /// \param g Group the task should be added to.
  /// \return
  /// task*  Pointer to the newly created task.
  static
  task* create_task(body_with_attrs<Fn,H>&& attrd_body, group *g) {
    auto attrs = attrd_body.get_attrs();
    return new
      mare_task_with_notify<Fn, H>(std::forward<Fn>(attrd_body.get_body()),
                                   std::forward<H>(attrd_body.
                                                   get_cancel_handler()),
                                   g, attrs);
  }

private:
  body_with_attrs<Fn,H>& _b;
};

/// Wraps functions, lambdas, and member functions used
/// to create tasks, even when they are also wrapped
/// within with_attrs.
template<typename Body>
struct body_wrapper : public body_wrapper_base<Body>
{
  typedef typename body_wrapper_base<Body>::traits body_traits;
  typedef typename body_traits::return_type return_type;

  /// Returns number of arguments required by body.
  /// \return size_t body arity.
  MARE_CONSTEXPR  static
  size_t get_arity() {
    return body_traits::arity;
  }

  /// Invokes body.
  /// \param args Body arguments.
  /// \return
  /// return_type Returns whatever the body returns.
  template<typename ...Args>
  MARE_CONSTEXPR  return_type operator()(Args... args) const {
    return (body_wrapper_base<Body>::get_fn())(args...);
  }

  /// Constructor.
  /// \param b Body to be wrapped.
  MARE_CONSTEXPR  body_wrapper(Body&& b):
    body_wrapper_base<Body>(std::forward<Body>(b))
  {}
};

/// \brief Easier way to get a body_wrapper
/// \return
/// body_wrapper<B> A body_wrapper object for b
template<typename B>
static body_wrapper<B> get_body_wrapper(B&& b)
{
  return body_wrapper<B>(std::forward<B>(b));
}

/// Removes constness and references from type T
template<typename T>
struct remove_cv_and_reference{
  typedef typename
  std::remove_cv<typename std::remove_reference<T>::type>::type type;
};

/// If T is a task_ptr, this struct provides the member constant
/// value equal true.
template<typename T>
struct is_task_ptr : std::integral_constant<
  bool,
  std::is_same<task_ptr, typename remove_cv_and_reference<T>::type>::value> {};

/// If T is an unsafe_task_ptr, this struct provides the member
/// constant value equal true. For any other type, value is false.
template<typename T >
struct is_unsafe_task_ptr : std::integral_constant<
  bool,
  std::is_same<unsafe_task_ptr,
               typename remove_cv_and_reference<T>::type>::value> {};

/// If T is an unsafe_task_ptr or a task_ptr, this struct provides the
/// member constant value equal true.
template< class T >
struct is_mare_task_ptr : std::integral_constant<
  bool,
  is_task_ptr<T>::value ||
  is_unsafe_task_ptr<T>::value> {};


/// Called when the programmer uses create_task(group_ptr, task_ptr)
/// or create_task(group_ptr, unsafe_task_ptr)
template<typename T, typename =
         typename std::enable_if<is_mare_task_ptr<T>::value>::type>
inline void launch_dispatch(T const& task)
{
  static group_ptr null_group_ptr;
  launch_dispatch(null_group_ptr, task);
}

/// Called when the programmer uses create_task(group_ptr, task_ptr)
/// or create_task(group_ptr, unsafe_task_ptr)
template<typename T, typename =
         typename std::enable_if<is_mare_task_ptr<T>::value>::type>
inline void launch_dispatch(group_ptr const& a_group, T const& a_task)
{
  auto t_ptr = c_ptr(a_task);
  auto g_ptr = c_ptr(a_group);

  MARE_API_ASSERT(t_ptr, "null task_ptr");
  t_ptr->launch(g_ptr);
}

/// Called when the programmer uses create_task(group_ptr, body)
template<typename Body,
         typename =
         typename std::enable_if<!is_mare_task_ptr<Body>::value>::type>
inline void launch_dispatch(group_ptr const& a_group, Body&& body)
{
  auto g_ptr = c_ptr(a_group);
  MARE_API_ASSERT(g_ptr, "null group_ptr");
  g_ptr->inc_task_counter();

  body_wrapper<Body> wrapper(std::forward<Body>(body));
  auto final_attrs = add_attr(wrapper.get_attrs(), attr::anonymous);

  task* t =  wrapper.create_task(g_ptr, final_attrs);
  t->set_runtime_owned();
  send_to_runtime(t);
}

/// Creates stub task
template<typename F>
static task_ptr create_stub_task(F f, task* pred = nullptr) {
  auto attrs = create_task_attrs(attr::stub, attr::non_cancelable);
  if (pred == nullptr) // yield
    attrs = add_attr(attrs, attr::yield);

  task_ptr stub = create_task(with_attrs(attrs, f));
  if (pred)
    task::add_task_dependence(pred, internal::c_ptr(stub));
  return stub;
}

/// Creates and launches stub_task
template<typename F>
static void launch_stub_task(F f, task* pred = nullptr) {
  auto stub = create_stub_task(f, pred);
  launch_dispatch(stub);
}

MARE_GCC_IGNORE_END("-Weffc++");

} //namespace internal

} //namespace mare
