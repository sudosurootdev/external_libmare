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
/** @file gpudevice.hh */
#pragma once

#ifdef HAVE_OPENCL

#include <mare/attr.hh>
#include <mare/range.hh>

#include <mare/internal/attr.hh>
#include <mare/internal/debug.hh>
#include <mare/internal/gpuopencl.hh>

namespace mare {

namespace internal {

class gpubuffer;
class gpukernel;
class gpuevent;

//gpu device currently based on opencl backend.
//reference counted and client would get a device_ptr.
class gpudevice : public ref_counted_object<gpudevice>
{
private:
  cl::Platform _ocl_platform;
  cl::Device _ocl_device;
  cl::Context _ocl_context;
  cl::CommandQueue _ocl_cmd_queue;
  cl::NDRange _ocl_global;
  cl::NDRange _ocl_local;
  cl::NDRange _ocl_offset;
  cl::Context get_context() {
    return _ocl_context;
  }
  void copy_to_device_async(const gpubuffer* buffer,
                            const void *ptr, size_t size,
                            cl::Event& write_event);
  void copy_from_device_async(const gpubuffer* buffer,
                              void *ptr, size_t size,
                              cl::Event& read_event );
  void* map_async(const gpubuffer* buffer, size_t size, cl::Event& read_event);
  void unmap_async(const gpubuffer* buffer, void* mapped_ptr,
                   cl::Event& write_event);
public:
  gpudevice(const cl::Platform& ocl_platform,
            const cl::Device& ocl_device) : _ocl_platform(ocl_platform),
                                            _ocl_device(ocl_device),
                                            _ocl_context(),
                                            _ocl_cmd_queue(),
                                            _ocl_global(),
                                            _ocl_local(),
                                            _ocl_offset() {
    //create context
    cl_int status;
    try {
      _ocl_context = cl::Context(ocl_device, NULL, NULL, NULL, &status);
    }
    catch (cl::Error err) {
      MARE_FATAL("cl::Context()->%d", err.err());
    }

    //create command queue.
    try {
      _ocl_cmd_queue = cl::CommandQueue(_ocl_context, _ocl_device, 0, &status);
    }
    catch (cl::Error err) {
      MARE_FATAL("cl::CommandQueue()->%d", err.err());
    }
  }

  explicit gpudevice(const cl::Device& ocl_device) :
    gpudevice(cl::Platform::getDefault(), ocl_device) { }

  gpudevice() :
    gpudevice(cl::Platform::getDefault(), cl::Device::getDefault()) { }

  template<size_t DIMS>
  void build_launch_args(const ::mare::range<DIMS>& r);

  template<size_t DIMS>
  gpuevent launch_kernel(const gpukernel* kernel,
                         const ::mare::range<DIMS>& r,
                         VECTOR_CLASS<cl::Event>* events=NULL);

  inline cl::Device get_impl() { return _ocl_device; }

  void flush() {
    cl_int status = _ocl_cmd_queue.finish();
    MARE_DLOG("cl::CommandQueue::finish()->%d",status);
    MARE_INTERNAL_ASSERT(status==CL_SUCCESS,
                         "cl::CommandQueue()::finish()->%d", status);
  }

  //function parameter used only for template argument deduction.
  //get(mare::device_attr::endian_little)
  //Attribute will be deduced as internal::device_attr_endian_little.
  //it would use the specialization
  //internal::device_attr<device_attr_endian_little>
  template<typename Attribute>
  auto get(Attribute) -> typename internal::device_attr<Attribute>::type {
    return _ocl_device.getInfo<internal::device_attr<Attribute>::cl_name>();
  }

  /// This functor is called by the shared_ptr object when the
  /// reference counter for the pointer gets to 0. If operator()
  /// returns true, then the shared_ptr can destroy the object
  /// pointed by the shared_ptr. If it returns false, it doesn't.
  ///
  /// All the pending commands to the device are flushed before
  /// returning from operator ()
  struct should_delete_functor {
    bool operator()(gpudevice* d) {
      MARE_INTERNAL_ASSERT((d != nullptr), "null device pointer");
      MARE_INTERNAL_ASSERT(d->use_count() == 0,
                           "Deleter called before counter hit 0.");

      //complete all command issued before deletion.
      cl_int status = d->_ocl_cmd_queue.finish();
      MARE_DLOG("cl::CommandQueue::finish()->%d",status);
      MARE_INTERNAL_ASSERT(status==CL_SUCCESS,
                           "cl::CommandQueue::finish()->%d",
                           status);
      if(status == CL_SUCCESS)
        return true;

      return false;
    }
  }; // should_delete_functor

  friend class gpubuffer;
  friend class gpukernel;
};

void get_devices(device_attr_device_type_gpu,
                 std::vector<device_ptr>* devices);

} //ns internal

} //ns mare

#endif // HAVE_OPENCL
