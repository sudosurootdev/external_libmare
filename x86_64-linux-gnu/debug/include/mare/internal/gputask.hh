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
/** @file gputask.hh */
#pragma once

#ifdef HAVE_OPENCL

#include <mare/buffer.hh>
#include <mare/gpukernel.hh>

#include <mare/internal/gpudevice.hh>
#include <mare/internal/gpuevent.hh>
#include <mare/internal/task.hh>

namespace mare {

template <typename T>
struct buffer_attrs {
  typedef T type;
};

MARE_GCC_IGNORE_BEGIN("-Weffc++");
//no template typedefs, inherit from
//base class instead.
template <typename T>
struct in : buffer_attrs<T> { };

template <typename T>
struct inout : buffer_attrs<T> { };

template <typename T>
struct out : buffer_attrs<T> { };

MARE_GCC_IGNORE_END("-Weffc++");

namespace internal {

namespace gpu_kernel_dispatch {

template<typename T>
struct cl_arg_setter
{
  static void set(internal::gpukernel* kernel,
                  size_t index,
                  T& arg) {
    MARE_DLOG("Setting kernel argument[%zu]: value type", index);
    kernel->set_arg(index, arg);
  }
};

// The following types sole purpose is to make the template code
// much more readable.
struct input_output_param{};
struct input_param{};
struct output_param{};
struct const_buffer{};
struct non_const_buffer{};

struct cl_arg_buffer_setter
{
  template<typename Buffer>
  static void set(device_ptr const& device,
                  internal::gpukernel* kernel,
                  size_t i,
                  Buffer& b,
                  input_output_param,
                  non_const_buffer) {

    MARE_DLOG("Setting kernel argument[%zu]: buffer type. Parameter: in_out,"
              " Argument: non_const. Copy-in: Y, Copy-out: Y", i);
    b->init_gpu_buffer(device);
    kernel->set_arg(i, b->get_gpu_buffer());
    b->copy_to_device(device);
    b->set_current_usage_inout();
  }

  template<typename Buffer>
  static void set(device_ptr const& device,
                  internal::gpukernel* kernel,
                  size_t i,
                  Buffer& b,
                  input_param,
                  non_const_buffer) {
    MARE_DLOG("Setting kernel argument[%zu]: buffer type. Parameter: in,"
              " Argument: non_const. Copy-in: Y, Copy-out: N", i);
    b->init_gpu_buffer(device);
    kernel->set_arg(i, b->get_gpu_buffer());
    b->copy_to_device(device);
    b->set_current_usage_in();
  }

  template<typename Buffer>
  static void set(device_ptr const& device,
                  internal::gpukernel* kernel,
                  size_t i,
                  Buffer& b,
                  input_param,
                  const_buffer) {
    MARE_DLOG("Setting kernel argument[%zu]: buffer type. Parameter: in,"
              " Argument: const. Copy-in: Y, Copy-out: N",i);
    b->init_gpu_buffer(device);
    kernel->set_arg(i, b->get_gpu_buffer());
    b->copy_to_device(device);
    //Argument is a const, so no copy-out will be performed.
    b->set_current_usage_in();
  }

  template<typename Buffer>
  static void set(device_ptr const& device,
                  internal::gpukernel* kernel,
                  size_t i,
                  Buffer& b,
                  output_param,
                  non_const_buffer) {
    MARE_DLOG("Setting kernel argument[%zu]: buffer type. Parameter: out,"
              " Argument: non_const. Copy-in:N, Copy-out:Y", i);
    b->init_gpu_buffer(device);
    kernel->set_arg(i, b->get_gpu_buffer());
    //setting the usage so that we can assert in buffer class
    b->set_current_usage_out();
    b->set_invalid();
  }
};

MARE_GCC_IGNORE_BEGIN("-Weffc++");

// The following templates check whether T is a buffer_ptr.
template <typename T>
struct is_buffer_ptr;

template<typename T>
struct is_buffer_ptr : public std::false_type {};

template <typename T>
struct is_buffer_ptr<buffer_ptr<T> > : public std::true_type {};

MARE_GCC_IGNORE_END("-Weffc++");


// The following templates define the kernel argument traits.
template<typename T> struct kernel_arg_ptr_traits;

template<typename T>
struct kernel_arg_ptr_traits<buffer_ptr<T>> {
  typedef T type;
  typedef non_const_buffer buffer_type;
  typedef typename std::remove_cv<T>::type naked_type;
};

template<typename T>
struct kernel_arg_ptr_traits<buffer_ptr<const T>> {
  typedef const T type;
  typedef const_buffer buffer_type;
  typedef typename std::remove_cv<T>::type naked_type;
};

template<typename T>
struct kernel_arg_ptr_traits<buffer_ptr<inout<T>>>;

template<typename T>
struct kernel_arg_ptr_traits<buffer_ptr<out<T>>>;

template<typename T>
struct kernel_arg_ptr_traits<buffer_ptr<in<T>>>;



// The following templates define the kernel parameter traits
template<typename T> struct kernel_param_ptr_traits;

template<typename T>
struct kernel_param_ptr_traits<buffer_ptr<T>> {
  typedef T type;
  typedef input_output_param direction;
  typedef typename std::remove_cv<T>::type naked_type;
};

template<typename T>
struct kernel_param_ptr_traits<buffer_ptr<const T>> {
  typedef const T type;
  typedef input_param direction;
  typedef typename std::remove_cv<T>::type naked_type;
};

template<typename T>
struct kernel_param_ptr_traits<buffer_ptr<inout<T>>> {
  typedef T type;
  typedef input_output_param direction;
  typedef typename std::remove_cv<T>::type naked_type;
};

template<typename T>
struct kernel_param_ptr_traits<buffer_ptr<inout<const T>>>;


template<typename T>
struct kernel_param_ptr_traits<buffer_ptr<in<T>>> {
  typedef T type;
  typedef input_param direction;
  typedef typename std::remove_cv<T>::type naked_type;
};

template<typename T>
struct kernel_param_ptr_traits<buffer_ptr<in<const T>>>;


template<typename T>
struct kernel_param_ptr_traits<buffer_ptr<out<T>>> {
  typedef T type;
  typedef output_param direction;
  typedef typename std::remove_cv<T>::type naked_type;
};

template<typename T>
struct kernel_param_ptr_traits<buffer_ptr<out<const T>>>;


/// Parses argument if it is not of buffer_ptr type.
/// @param index Argument index
/// @param arg Reference to argument
template<typename Param, typename Arg>
void parse_normal_arg(device_ptr const&,
                      internal::gpukernel* kernel,
                      size_t index,
                      Arg& arg)
{
  // Make sure that we weren't expecting a buffer_ptr
  static_assert(!is_buffer_ptr<Param>::value, "Expected buffer_ptr argument");
  cl_arg_setter<Arg>::set(kernel, index, arg);
}

/// Parses argument if arg is a buffer_ptr
/// @param index Argument index
/// @param arg Reference to argument
template<typename Param, typename Arg>
void parse_buffer_ptr_arg(device_ptr const& device,
                          internal::gpukernel* kernel,
                          size_t index,
                          Arg& arg)
{
  // Check first whether the parameter is also a buffer_ptr
  static_assert(is_buffer_ptr<Param>::value, "Expected buffer_ptr argument");

  typedef kernel_param_ptr_traits<Param> param_traits;
  typedef typename param_traits::naked_type naked_param_type;
  typedef typename param_traits::direction direction;

  typedef kernel_arg_ptr_traits<Arg> arg_traits;
  typedef typename arg_traits::naked_type naked_arg_type;
  typedef typename arg_traits::buffer_type buffer_type;

  static_assert(std::is_same<naked_param_type, naked_arg_type>::value,
                "Incompatible buffer_ptr types");

  cl_arg_buffer_setter::set(device,
                            kernel,
                            index,
                            arg,
                            direction(),
                            buffer_type());
}


// Dispatcher methods. They use their last argument to dispatch
// to parse_buffer_ptr or parse_normal_arg
template<typename Param, typename Arg>
void parse_arg_dispatch(device_ptr const& device,
                        internal::gpukernel* kernel,
                        size_t index,
                        Arg& arg,
                        std::false_type)
{
  parse_normal_arg<Param, Arg>(device, kernel, index, arg);
}
template<typename Param, typename Arg>
void parse_arg_dispatch(device_ptr const& device,
                        internal::gpukernel* kernel,
                        size_t index,
                        Arg& arg,
                        std::true_type)
{
  parse_buffer_ptr_arg<Param, Arg>(device, kernel, index, arg);
}

/// The next templates traverse the the argument list
/// and call prepare_arg for each of the arguments.
template<typename Params, typename Args, size_t index = 0>
typename std::enable_if<index == std::tuple_size<Args>::value, void>::type
prepare_args(device_ptr const&, internal::gpukernel*, Args&&)
{

}

template<typename Params, typename Args, size_t index =0>
typename std::enable_if<index < std::tuple_size<Args>::value, void>::type
prepare_args(device_ptr const& device,
             internal::gpukernel* kernel,
             Args&& args)
{

  // Fail if the number of paraments is different than the number of arguments.
  static_assert(std::tuple_size<Args>::value == std::tuple_size<Params>::value,
                "Number of parameters is different to number of arguments");

  typedef typename std::tuple_element<index, Params>::type param_type;
  typedef typename std::tuple_element<index, Args>::type arg_type;
  typedef typename is_buffer_ptr<arg_type>::type is_arg_a_buffer_ptr;

  parse_arg_dispatch<param_type, arg_type> (device,
                                            kernel,
                                            index,
                                            std::get<index>(args),
                                            is_arg_a_buffer_ptr());

  prepare_args<Params, Args, index + 1>(device, kernel,
                                        std::forward<Args>(args));
}

} // namespace gpu_kernel_dispatch

template<size_t DIMS, typename Fn, typename Params, typename Args>
class gputask : public task {

private:
  Args _kernel_args;
  gpuevent _completion_event;
  gpukernel* _kernel;
  gpudevice* _d_ptr;
  ::mare::range<DIMS> _range;
  device_ptr const& _device;
  Fn& _fn; //cpu version

  MARE_DELETE_METHOD(gputask(gputask const&));
  MARE_DELETE_METHOD(gputask(gputask&&));
  MARE_DELETE_METHOD(gputask& operator=(gputask const&));
  MARE_DELETE_METHOD(gputask& operator=(gputask&&));


public:
  template <typename Kernel>
  gputask(device_ptr const& device,
          const ::mare::range<DIMS>& r,
          Fn& f,
          task_attrs a,
          Kernel kernel,
          Args& kernel_args) : task(nullptr, a),
                               _kernel_args(kernel_args),
                               _completion_event(),
                               _kernel(nullptr),
                               _d_ptr(nullptr),
                               _range(r),
                               _device(device),
                               _fn(f) {
    _d_ptr = internal::c_ptr(device);
    MARE_INTERNAL_ASSERT((_d_ptr != nullptr), "null device");
    auto k_ptr = internal::c_ptr(kernel);
    MARE_INTERNAL_ASSERT((k_ptr != nullptr), "null kernel");
    _kernel = k_ptr->get_kernel();
    MARE_DLOG("kernel: %p", _kernel);
  }

  static void CL_CALLBACK completion_callback(cl_event,
                                              cl_int, void* user_data) {
    auto this_task = static_cast<gputask*>(user_data);
    MARE_DLOG("Callback: %p, %s", this_task, this_task->to_string().c_str());
    bool in_utcache = this_task->get_state(std::memory_order_acquire).
                                           in_utcache();
    this_task->task_completed(false, in_utcache);
  }

  virtual void execute();

  virtual void cancel_notify() {
  }
};

template<size_t DIMS, typename Fn, typename Params, typename Args>
void
gputask<DIMS, Fn, Params, Args>::execute()
{
    MARE_INTERNAL_ASSERT(get_state().is_running(),
                         "Can't execute task %p: %s",
                         this, to_string().c_str());
    MARE_DLOG("enqueuing OpenCL task %p", this);

    gpu_kernel_dispatch::
      prepare_args<Params, Args>(_device,
                                 _kernel,
                                 std::forward<Args>(_kernel_args));
    _completion_event = _d_ptr->launch_kernel(_kernel, _range);
    _completion_event.get_impl().setCallback(CL_COMPLETE,
                                             &completion_callback,
                                             this);
}


} // ns internal

} // ns mare
#endif // HAVE_OPENCL
