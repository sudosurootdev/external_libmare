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
/** @file gpukernel.hh */
#pragma once

#ifdef HAVE_OPENCL

#include <mare/internal/debug.hh>
#include <mare/internal/gpubuffer.hh>
#include <mare/internal/gpudevice.hh>
#include <mare/internal/gpuopencl.hh>

namespace mare {

namespace internal {

// GPU devices on the platform.
extern std::vector<mare::device_ptr> s_dev_ptrs;

//No concept of Program since tasks map more naturally
//to a single kernel rather than a collection of kernels.
class gpukernel
{
private:
  cl::Program _ocl_program;
  cl::Kernel _ocl_kernel;
public:
  gpukernel(device_ptr const& device,
            const std::string& task_str,
            const std::string& task_name) : _ocl_program(),
                                            _ocl_kernel() {
    auto d_ptr = internal::c_ptr(device);
    MARE_INTERNAL_ASSERT((d_ptr != nullptr), "null device ptr");

    //create program.
    cl_int status;
    try {
      _ocl_program = cl::Program(d_ptr->get_context(),
                                 task_str, false, &status);

      //build program.
      VECTOR_CLASS<cl::Device> devices;
      devices.push_back(d_ptr->get_impl());
      status = _ocl_program.build(devices);
    }
    catch(cl::Error err) {
      cl::STRING_CLASS build_log;
      _ocl_program.getBuildInfo(d_ptr->get_impl(),
                                CL_PROGRAM_BUILD_LOG, &build_log);
      MARE_FATAL("cl::Program::build()->%d\n build_log: %s",
                  err.err(), build_log.c_str());
    }

    //create kernel
    try {
      _ocl_kernel = cl::Kernel(_ocl_program, task_name.c_str(), &status);
    }
    catch(cl::Error err) {
      MARE_FATAL("cl::Kernel()->%d", err.err());
    }
  }

  inline const cl::Kernel get_impl() const {
    return _ocl_kernel;
  }

  //handle buffer types.
  inline void set_arg(size_t arg_index, gpubuffer_ptr const& gpubuffer) {
    auto b_ptr = internal::c_ptr(gpubuffer);
    MARE_INTERNAL_ASSERT((b_ptr != nullptr), "null gpubuffer pointer");

    try {
      //set kernel arg
      cl_int status = _ocl_kernel.setArg(arg_index, b_ptr->get_impl());
      MARE_UNUSED(status);
      MARE_DLOG("cl::Kernel::setArg(%zu, %p)->%d", arg_index, b_ptr, status);
    }
    catch (cl::Error err) {
      MARE_FATAL("cl::Kernel::setArg(%zu, %p)->%d",
                 arg_index, b_ptr, err.err());
    }
  }

  //handle value types.
  template<typename T>
  void set_arg(size_t arg_index, T value) {
    try {
      cl_int status = _ocl_kernel.setArg(arg_index, sizeof(T), &value);
      MARE_UNUSED(status);
      MARE_DLOG("cl::Kernel::setArg(%zu, %zu, %p)->%d",
                arg_index,
                sizeof(T),
                &value,
                status);
    }
    catch (cl::Error err) {
      MARE_FATAL("cl::Kernel::setArg(%zu, %zu, %p)->%d",
                 arg_index, sizeof(T), &value, err.err());

    }
  }

  //Each element from a tuple is extracted and an index is
  //incremented, set_args(with empty body) is only enabled
  //when index is equal to length of Kargs, this terminates
  //the recursion.
  template<size_t index = 0, typename... Kargs>
  typename std::enable_if<(index == sizeof...(Kargs)), void>::type
   set_args(const std::tuple<Kargs...>&) {

  }

  template<size_t index =0, typename... Kargs>
  typename std::enable_if<(index < sizeof...(Kargs)), void>::type
   set_args(const std::tuple<Kargs...>& t) {
    set_arg(index, std::get<index>(t));
    set_args<index+1, Kargs...>(t);
  }
};

} //ns internal

// squelch GCC complaints about non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");

//mare::create_kernel returns a shared_ptr to this class.
template<typename...Params>
class gpukernel : public mare::internal::
                         ref_counted_object<gpukernel<Params...> >,
                  public internal::gpu_kernel_base
{

MARE_GCC_IGNORE_END("-Weffc++");

private:
  mare::internal::gpukernel* _kernel;

  MARE_DELETE_METHOD(gpukernel(gpukernel const&));
  MARE_DELETE_METHOD(gpukernel(gpukernel&&));
  MARE_DELETE_METHOD(gpukernel& operator=(gpukernel const&));
  MARE_DELETE_METHOD(gpukernel& operator=(gpukernel&&));
public:
  typedef std::tuple<Params...> parameters;

  gpukernel(const std::string& kernel_str,
            const std::string& kernel_name) :
    _kernel(nullptr) {

    //get devices.
    MARE_INTERNAL_ASSERT(!mare::internal::s_dev_ptrs.empty(),
                         "No GPU devices found on the platform");
    _kernel = new mare::internal::gpukernel(mare::internal::s_dev_ptrs[0],
                                            kernel_str,
                                            kernel_name);
    MARE_DLOG("kernel_name: %s, %p", kernel_name.c_str(), _kernel);
  }

  mare::internal::gpukernel* get_kernel() { return _kernel; }
  struct should_delete_functor {
    bool operator()(gpukernel<Params...>* k) {

      MARE_API_DETAILED_ASSERT((k != nullptr), "null kernel pointer");
      MARE_API_DETAILED_ASSERT(k->use_count() == 0,
                               "Deleter called before counter hit 0.");
      delete k->get_kernel();
      return true;
    }
  }; // should_delete_functor};

};

} //ns mare

#endif //HAVE_OPENCL
