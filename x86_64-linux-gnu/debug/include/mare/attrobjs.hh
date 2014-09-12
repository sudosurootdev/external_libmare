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
/** @file attrobjs.hh */
#pragma once

#include <cstdint>
#include <tuple>
#include <utility>

#include <mare/common.hh>

#include <mare/internal/functiontraits.hh>

// Forward declarations
namespace mare  {

class task_attrs;

/** @cond HIDDEN */
template <typename... TS> class body_with_attrs;
#ifdef HAVE_OPENCL
template <typename... TS> class body_with_attrs_gpu;

template <typename... Kargs>
class gpu_kernel;

template <typename Body, typename KernelPtr, typename... Kargs>
MARE_CONSTEXPR
body_with_attrs_gpu<Body, KernelPtr, Kargs...>  with_attrs_gpu_internal(
                                                task_attrs const& attrs,
                                                Body&& body,
                                                KernelPtr kernel,
                                                Kargs... args);

MARE_CLANG_IGNORE_BEGIN("-Wpredefined-identifier-outside-function");
MARE_CLANG_IGNORE_BEGIN("-Wunneeded-internal-declaration");

static auto default_cpu_kernel = []() {
  MARE_UNIMPLEMENTED("cpu version of task not provided");
};

typedef decltype(default_cpu_kernel) CpuKernelType;

MARE_CLANG_IGNORE_END("-Wunneeded-internal-declaration");
MARE_CLANG_IGNORE_END("-Wpredefined-identifier-outside-function");

template <typename KernelPtr, typename... Kargs>
MARE_CONSTEXPR body_with_attrs_gpu<CpuKernelType, KernelPtr, Kargs...>
with_attrs_gpu(task_attrs const& attrs, KernelPtr kernel, Kargs... args);


#endif // HAVE_OPENCL
/** @endcond */

template <typename Body>
MARE_CONSTEXPR body_with_attrs<Body>
with_attrs_cpu(task_attrs const& attrs, Body&& body);

/** @cond HIDDEN */

template <typename Body>
MARE_CONSTEXPR body_with_attrs<Body>
with_attrs_cpu(task_attrs const& attrs, Body&& body, std::nullptr_t);
/** @endcond */


template <typename Body, typename Cancel_Handler>
MARE_CONSTEXPR body_with_attrs<Body, Cancel_Handler>
with_attrs_cpu(task_attrs const& attrs,
               Body&& body,
               Cancel_Handler&& handler);

/** @cond HIDDEN */

//dispatcher for with_attrs.
template<typename Body, typename T, typename... Kargs>
struct with_attrs_dispatcher;

/** @endcond */

MARE_CONSTEXPR task_attrs create_task_attrs();

} // namespace mare

/** @cond */
// These forward declarations need to be outside of the mare namespace
MARE_CONSTEXPR bool
operator== (mare::task_attrs const& a, mare::task_attrs const& b);
MARE_CONSTEXPR bool
operator!= (mare::task_attrs const& a, mare::task_attrs const& b);
/** @endcond */

namespace mare {

/** @addtogroup tasks_doc
@{ */
/**
    Stores the attributes for a task. Use
    create_task_attrs(...)  to create a task_attrs object.
*/
class task_attrs {
public:
/**
    Copy Constructor.
    @param other Original task_attrs.
*/
  MARE_CONSTEXPR task_attrs(task_attrs const& other):
    _mask(other._mask)
  {}

/** Copy assignment.
    @param other Original task_attrs.
*/
  task_attrs& operator=(task_attrs const& other) {
    _mask = other._mask;
    return *this;
  }

#ifndef _MSC_VER
  friend constexpr bool ::operator== (task_attrs const& a,
                                      task_attrs const& b);
  friend constexpr bool ::operator!= (task_attrs const& a,
                                      task_attrs const& b);
#endif

#ifndef _MSC_VER
private:
  constexpr
#endif
  task_attrs():_mask(0){}

  MARE_CONSTEXPR task_attrs(std::int32_t mask):_mask(mask){}

  std::int32_t _mask;

#ifndef _MSC_VER
  friend constexpr task_attrs create_task_attrs();

  template<typename Attribute, typename...Attributes>
  friend constexpr task_attrs create_task_attrs(Attribute const&,
                                                Attributes const& ...);

  template<typename Attribute>
  friend constexpr bool has_attr(task_attrs const& attrs,
                                 Attribute const& attr);

  template<typename Attribute>
  friend constexpr task_attrs remove_attr(task_attrs const& attrs,
                                          Attribute const& attr);

  template<typename Attribute>
  friend const task_attrs add_attr(task_attrs const& attrs,
                                   Attribute const& attr);
#endif
};

/**
    Temporarily stores attributes and other information needed
    to create a task. Create them using the template method
    mare::with_attrs.
*/
template <typename Body>
class body_with_attrs<Body>
{
public:
  /** Traits of the body. */
  typedef internal::function_traits<Body> body_traits;
  /** @return type of the body. */
  typedef typename body_traits::return_type return_type;

  /** Returns body attributes.
      @return
      task_attrs -- body attributes.
  */
  MARE_CONSTEXPR task_attrs const& get_attrs() const { return _attrs;}

  /** Returns body.
      @return
      Body -- body.
  */
  MARE_CONSTEXPR Body& get_body() const { return _body; }

  /** Calls body.
      @return
      @param args Arguments to be passed to the body.

      return_type -- Returns the value returned by the body.
  */
  template<typename ...Args>
  MARE_CONSTEXPR return_type operator()(Args... args) const {
    return (get_body())(args...);
  }

#ifndef _MSC_VER
private:
  constexpr
#endif
  body_with_attrs(task_attrs const& attrs, Body&& body):
    _attrs(attrs),
    _body(body){
  }

  task_attrs const& _attrs;
  Body& _body;

#ifndef _MSC_VER

/** @cond HIDDEN */

  friend ::mare::body_with_attrs<Body>
  with_attrs_cpu<>(::mare::task_attrs const& attrs, Body&& body);
  friend ::mare::body_with_attrs<Body>
  with_attrs_cpu<>(::mare::task_attrs const& attrs, Body&& body,
                   std::nullptr_t);
/** @endcond */

#endif
};

/** @cond HIDDEN */

// Empty class for type deduction in pfor_each.
// Used to select a GPU version of pfor_each.
class body_with_attrs_base_gpu { };

#ifdef HAVE_OPENCL
// squelch GCC complaints about non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");

/** @endcond */

/**
   Temporarily stores attributes and other information needed
   to create a gpu task. Create them using the template method
   mare::with_attrs.
*/
template <typename Body,
          typename KernelPtr,
          typename... Kargs>
class body_with_attrs_gpu<Body, KernelPtr, Kargs...> :
                                               public body_with_attrs_base_gpu
{

MARE_GCC_IGNORE_END("-Weffc++");

public:
  /** Traits of the body. */
  typedef internal::function_traits<Body> body_traits;
  /** Return type of the body. */
  typedef typename body_traits::return_type return_type;
  /** Kernel parameters */
  typedef typename KernelPtr::type::parameters kernel_parameters;
  /** Kernel arguments */
  typedef std::tuple<Kargs...> kernel_arguments;


  /** Returns body attributes
      @return
      task_attrs --- body attributes
  */
  MARE_CONSTEXPR task_attrs const& get_attrs() const { return _attrs;}

  /** Returns body
      @return
      Body --- body
  */
  MARE_CONSTEXPR Body& get_body() const { return _body; }

  /** Calls body
      @param args Arguments to be passed to the body.

      @return return_type --- Returns the value returned by the body.
  */
  template<typename ...Args>
  MARE_CONSTEXPR return_type operator()(Args... args) const {
    return (get_body())(args...);
  }
  /** Returns the kernel ptr passed to a gpu task. The kernel ptr
      points to a template object, the template parameters has the
      same signature as the OpenCL kernel parameters used in the current
      gpu task. This is used to check at compile time if the kernel
      arguments passed to gpu task have same type as the kernel parameters.

      @return Returns the kernel arguments passed to a gpu task.
      @sa create_kernel()
  */
  KernelPtr& get_gpu_kernel() { return _kernel; }

  /** Returns the kernel arguments passed to a gpu task.
      @return Returns the kernel arguments passed to a gpu task.
  */
  kernel_arguments& get_kernel_args() { return _kargs; }
private:
  MARE_CONSTEXPR body_with_attrs_gpu(task_attrs const& attrs, Body&& body,
                                     KernelPtr kernel, Kargs&&... args):
    _attrs(attrs),
    _body(body),
    _kernel(kernel),
    _kargs(std::tuple<Kargs...>(std::forward<Kargs>(args)...)) {  }

  task_attrs const& _attrs;
  Body& _body;
  KernelPtr _kernel;
  //store the variadic template parameters for later use.
  kernel_arguments _kargs;

/** @cond HIDDEN */
#ifndef _MSC_VER
  friend ::mare::body_with_attrs_gpu<Body, KernelPtr, Kargs...>
  with_attrs_gpu_internal<>(::mare::task_attrs const& attrs, Body&& body,
                        KernelPtr kernel, Kargs... args);

  friend ::mare::body_with_attrs_gpu<CpuKernelType,
                                     KernelPtr, Kargs...>
  with_attrs_gpu<>(::mare::task_attrs const& attrs,
               KernelPtr kernel, Kargs... args);

#endif
/** @endcond  */

};

/** @cond HIDDEN */
#endif // HAVE_OPENCL
/** @endcond **/

/**
    Temporarily stores attributes and other information needed
    to create a task. Create them using the template method
    with_attrs.
*/
template <typename Body, typename Cancel_Handler>
class body_with_attrs<Body, Cancel_Handler>
{
public:

  /** Traits of the body. */
  typedef internal::function_traits<Body> body_traits;
  /** Return type of the body. */
  typedef typename body_traits::return_type return_type;

  /** Traits of the cancel handler. */
  typedef internal::function_traits<Cancel_Handler> cancel_handler_traits;
  /** Return type of the cancel handler. */
  typedef typename cancel_handler_traits::return_type
    cancel_handler_return_type;

  /** Returns body attributes.
      @return
      task_attrs -- Body attributes.
  */
  MARE_CONSTEXPR task_attrs const& get_attrs() const {
    return _attrs;
  }

  /** Returns body.
      @return
      Body -- body.
  */
  MARE_CONSTEXPR Body& get_body() const{
    return _body;
  }

  /** Returns cancel handler.
      @return
      cancel_handler_type -- Cancel_handler.
  */
  MARE_CONSTEXPR Cancel_Handler& get_cancel_handler() const {
    return _handler;
  }

  /** Calls body
      @return
      @param args Arguments to be passed to the body.
      return_type -- Returns the value returned by the body.
  */
  template<typename ...Args>
  MARE_CONSTEXPR return_type operator()(Args... args) const {
    return (get_body())(args...);
  }

#ifndef _MSC_VER
private:
  constexpr
#endif
  body_with_attrs(task_attrs const& attrs, Body&& body,
                  Cancel_Handler&& handler):
    _attrs(attrs),
    _body(body),
    _handler(handler){}

  task_attrs const& _attrs;
  Body& _body;
  Cancel_Handler& _handler;

/** @cond HIDDEN */
#ifndef _MSC_VER
  friend body_with_attrs<Body, Cancel_Handler>
  with_attrs_cpu<>(task_attrs const& attrs, Body&& body,
                   Cancel_Handler&& handler);
#endif
/** @endcond */
};

#ifdef ONLY_FOR_DOXYGEN
#error The compiler should not see these methods


/**
    Creates a body_with_attrs object that encapsulates the task
    body and the task attributes.

    @param attrs Attributes.
    @param body Task body.

    @return body_with_attrs with task attributes, task body, but no cancel
    handler.
*/
template <typename Body>
MARE_CONSTEXPR body_with_attrs<Body>
with_attrs(task_attrs const& attrs, Body&& body);

/**
    Creates a body_with_attrs object that encapsulates the task
    body, the cancel handler, and the task attributes.

    @param attrs Attributes.
    @param body Task body.
    @param handler Cancel handler.

    @return body_with_attrs with task attributes, task body, and cancel
    handler body.
*/
template <typename Body, typename Cancel_Handler>
MARE_CONSTEXPR body_with_attrs<Body, Cancel_Handler>
with_attrs(task_attrs const& attrs, Body&& body, Cancel_Handler &&handler);

/**
    Creates a body_with_attrs object that encapsulates the task attributes,
    Kernel ptr, the OpenCL C kernel string, and the kernel arguments.

    @param attrs Attributes.
    @param kernel Kernel pointer which points to a template kernel object.
    @param args Kernel arguments to be passed to the OpenCL kernel.

    @return body_with_attrs with task attributes, kernel ptr, OpenCL C
    kernel string and kernel arguments.

    @sa create_kernel
*/
template<typename KernelPtr, typename... Kargs>
MARE_CONSTEXPR body_with_attrs_gpu<CpuKernelType, KernelPtr, Kargs...>
with_attrs(task_attrs const& attrs,
           KernelPtr kernel, Kargs... args);

#endif //ONLY_FOR_DOXYGEN


/** @cond HIDDEN*/

template <typename Body>
MARE_CONSTEXPR body_with_attrs<Body>
with_attrs_cpu(task_attrs const& attrs, Body&& body)
{
  return body_with_attrs<Body>(attrs, std::forward<Body>(body));
}

template <typename Body, typename Cancel_Handler>
MARE_CONSTEXPR body_with_attrs<Body, Cancel_Handler>
with_attrs_cpu(task_attrs const& attrs, Body&& body, Cancel_Handler &&handler)
{
  return body_with_attrs<Body, Cancel_Handler>
    (attrs, std::forward<Body>(body),
     std::forward<Cancel_Handler>(handler));
}

#ifdef HAVE_OPENCL

template<typename Body, typename KernelPtr, typename... Kargs>
MARE_CONSTEXPR body_with_attrs_gpu<Body, KernelPtr, Kargs...>
with_attrs_gpu_internal(task_attrs const& attrs, Body&& body,
           KernelPtr kernel, Kargs... args)
{
  return body_with_attrs_gpu<Body,
                             KernelPtr,
                             Kargs...> (attrs, std::forward<Body>(body),
                                        kernel,
                                        std::forward<Kargs>(args)...);
}

template<typename KernelPtr, typename... Kargs>
MARE_CONSTEXPR
body_with_attrs_gpu<CpuKernelType, KernelPtr, Kargs...>
with_attrs_gpu(task_attrs const& attrs,
           KernelPtr kernel, Kargs... args)
{
  return with_attrs_gpu_internal(attrs,
                             std::forward<CpuKernelType>
                                          (default_cpu_kernel),
                             kernel,
                             std::forward<Kargs>(args)...);
}

template<typename Body, typename... Kargs>
struct with_attrs_dispatcher<Body, std::true_type, Kargs...>
{
  static auto dispatch(task_attrs const& attrs, Body&& body, Kargs... args)
    ->decltype(with_attrs_gpu(attrs,
                              std::forward<Body>(body),
                              std::forward<Kargs>(args)...))
  {
    return with_attrs_gpu(attrs,
                          std::forward<Body>(body),
                          std::forward<Kargs>(args)...);
  }
};

#endif //HAVE_OPENCL

//We don't the return type of with_attrs_cpu since it can pick any of
//the three overloaded functions above, we use auto & decltype.
template<typename Body, typename... Kargs>
struct with_attrs_dispatcher<Body, std::false_type, Kargs...>
{
  static auto dispatch(task_attrs const& attrs, Body&& body, Kargs&&... args)
    ->decltype(with_attrs_cpu(attrs,
                              std::forward<Body>(body),
                              std::forward<Kargs>(args)...))
  {
    return with_attrs_cpu(attrs,
                          std::forward<Body>(body),
                          std::forward<Kargs>(args)...);
  }
};

template<typename Body, typename = void>
struct is_gpu_kernel
{
  typedef Body type;
  typedef std::false_type result_type;
};

//Way to identify that body is gpu_kernel type
//For gpu tasks, Body will be mare_shared_ptr,
//we get the type from mare_shared_ptr and
//check if type derives from gpu_kernel_base; this
//will be true only on the gpu path.
template<typename Body>
struct is_gpu_kernel<
         Body,
         typename is_gpu_kernel<void, typename Body::type>::type>
{
  typedef typename Body::type type;
  typedef typename std::is_base_of<internal::gpu_kernel_base,
                                   typename Body::type>::type result_type;
};

template<typename Body, typename... Kargs>
MARE_CONSTEXPR auto
with_attrs(task_attrs const& attrs, Body&& body, Kargs&&... args)
  ->decltype(with_attrs_dispatcher<
               Body, typename is_gpu_kernel<
                      typename std::remove_reference<Body>::type>::result_type,
               Kargs...>::dispatch(attrs,
                                   std::forward<Body>(body),
                                   std::forward<Kargs>(args)...))
{  return with_attrs_dispatcher<
               Body, typename is_gpu_kernel<
                      typename std::remove_reference<Body>::type>::result_type,
               Kargs...>::dispatch(attrs,
                                   std::forward<Body>(body),
                                   std::forward<Kargs>(args)...);
}
/** @endcond */

/** @} */ /* end_addtogroup tasks_doc */

} //namespace mare

/** @addtogroup attributes
@{ */
/**
    Checks whether two task_attrs objects are equivalent.

    @param a First task_attrs object.
    @param b Second task_attrs object.

    @return
    TRUE -- The two task_attrs objects are equivalent.
    FALSE -- The two task_attrs objects are not equivalent.
*/
inline MARE_CONSTEXPR
bool operator== (mare::task_attrs const& a, mare::task_attrs const& b) {
  return a._mask == b._mask;
}

/**
    Checks whether two task_attrs objects are equivalent.

    @param a First task_attrs object.
    @param b Second task_attrs object.

    @return
    TRUE -- The two task_attrs objects are not equivalent.
    FALSE -- The two task_attrs objects are equivalent.
*/
inline MARE_CONSTEXPR
bool operator!= (mare::task_attrs const& a, mare::task_attrs const& b) {
  return a._mask != b._mask;

}

/** @} */ /* end_addtogroup attributes */
