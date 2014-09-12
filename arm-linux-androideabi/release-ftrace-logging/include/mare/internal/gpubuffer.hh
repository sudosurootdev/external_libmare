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
/** @file gpubuffer.hh */
#pragma once

#ifdef HAVE_OPENCL

#include <mare/internal/gpudevice.hh>
#include <mare/internal/gpuopencl.hh>

namespace mare {

namespace internal {

// forwarding declaration
namespace test {
class gpubuffer_tests;
} //namespace test


//gpu buffer currently based on opencl backend.
//reference counted and client would get a gpubuffer_ptr.
class gpubuffer : public ref_counted_object<gpubuffer>
{
private:
  gpudevice* _device;
  size_t _size;
  cl::Buffer _ocl_buffer;
  cl::Event _write_event;
  cl::Event _read_event;
  void* _mapped_ptr;
  void* _host_ptr;
  MARE_DELETE_METHOD(gpubuffer(gpubuffer const&));
  MARE_DELETE_METHOD(gpubuffer(gpubuffer&&));
  MARE_DELETE_METHOD(gpubuffer& operator=(gpubuffer const&));
  MARE_DELETE_METHOD(gpubuffer& operator=(gpubuffer&&));
public:
  //Create device memory,UHP
  template<typename Attribute>
  gpubuffer(gpudevice* d, Attribute,
            const void* ptr, size_t sz) : _device(d),
                                          _size(sz),
                                          _ocl_buffer(),
                                          _write_event(),
                                          _read_event(),
                                          _mapped_ptr(nullptr),
                                          _host_ptr(const_cast<void*>(ptr)) {
    //create buffer.
    cl_int status;
    unsigned long long flags = internal::buffer_attr<Attribute>::cl_name |
                           CL_MEM_USE_HOST_PTR;
    try {
      _ocl_buffer =  cl::Buffer(_device->get_context(),
                                flags,
                                _size,
                                _host_ptr,
                                &status);
      MARE_DLOG("cl::Buffer(%llu, %zu, %p)->%d",
                flags, _size, ptr, status);
    }
    catch (cl::Error err) {
      MARE_FATAL("cl::Buffer(%llu, %zu, %p)->%d,",
                 flags, _size, ptr, err.err());
    }
  }

  //Create device memory, uninitialized,AHP
  template<typename Attribute>
  gpubuffer(gpudevice* d, Attribute, size_t sz) :_device(d),
                                                 _size(sz),
                                                 _ocl_buffer(),
                                                 _write_event(),
                                                 _read_event(),
                                                 _mapped_ptr(nullptr),
                                                 _host_ptr(nullptr) {
    //create buffer.
    cl_int status;
    unsigned long long flags = internal::buffer_attr<Attribute>::cl_name |
      CL_MEM_ALLOC_HOST_PTR;
    try {
      _ocl_buffer =  cl::Buffer(_device->get_context(),
                                flags,
                                _size,
                                NULL,
                                &status);
      MARE_DLOG("cl::Buffer(%llu, %zu, %p)->%d",
                flags, _size, nullptr, status);
    }
    catch (cl::Error err) {
      MARE_FATAL("cl::Buffer(%llu, %zu, %p)->%d",
                  flags, _size, nullptr, err.err());
    }
  }

  inline void copy_to_device();

  inline void* copy_from_device();

  inline void wait_for_read() {
    _read_event.wait();
  }

  inline void wait_for_write() {
    _write_event.wait();
  }

  inline const cl::Buffer get_impl() const {
    return _ocl_buffer;
  }

  inline size_t size() const {
    return _size;
  }

  ~gpubuffer() {
    if(_mapped_ptr) {
      copy_to_device();
      _device->flush();
    }
  }

  /// This functor is called by the shared_ptr object when the
  /// reference counter for the pointer gets to 0. If operator()
  /// returns true, then the shared_ptr can destroy the object
  /// pointed by the shared_ptr. If it returns false, it doesn't.
  ///
  /// The reason for this struct is that we can't delete unlaunched
  /// tasks with predecessors, because tasks dependences do not count
  /// towards the number of references.
  struct should_delete_functor {
    bool operator()(gpubuffer* b) {

      MARE_API_DETAILED_ASSERT((b != nullptr), "null buffer pointer");
      MARE_API_DETAILED_ASSERT(b->use_count() == 0,
                               "Deleter called before counter hit 0.");

      //Read the data back into host buffer
      return true;
    }
  }; // should_delete_functor
  friend class test::gpubuffer_tests;
};

//Copy data to gpu buffer.
void
gpubuffer::copy_to_device()
{
    if(_host_ptr == nullptr && _mapped_ptr == nullptr)
      return;

    if(_host_ptr) {
      _device->copy_to_device_async(this, _host_ptr, _size, _write_event);
    }
    else if(_mapped_ptr) {
      _device->unmap_async(this, _mapped_ptr, _write_event);
      _mapped_ptr = nullptr;
    }
    wait_for_write();
}

//Copy data from gpu buffer.
void*
gpubuffer::copy_from_device()
{
    MARE_INTERNAL_ASSERT(!(_host_ptr && _mapped_ptr),
                         "both _host_ptr and _mapped_ptr are non-null");
    if(_host_ptr) {
      _device->copy_from_device_async(this, _host_ptr, _size, _read_event);
    }
    else if(_mapped_ptr == nullptr) {
      _mapped_ptr = _device->map_async(this, _size, _read_event);
    }
    wait_for_read();
    return (_host_ptr) ? _host_ptr : _mapped_ptr;
}

}// ns internal

template<typename Attribute>
inline gpubuffer_ptr create_buffer(device_ptr const& device, Attribute attr,
                                   const void *ptr, size_t size)
{
  auto d_ptr = internal::c_ptr(device);
  MARE_API_ASSERT((d_ptr != nullptr), "null device_ptr");
  internal::gpubuffer* b_ptr = new internal::gpubuffer(d_ptr, attr, ptr, size);
  MARE_DLOG("Creating mare::internal::gpubuffer %p", b_ptr);
  return mare::gpubuffer_ptr(b_ptr, gpubuffer_ptr::ref_policy::DO_INITIAL_REF);
}

template<typename Attribute>
inline gpubuffer_ptr create_buffer(device_ptr const& device,
                                   Attribute attr, size_t size)
{
  auto d_ptr = internal::c_ptr(device);
  MARE_API_ASSERT((d_ptr != nullptr), "null device_ptr");
  internal::gpubuffer* b_ptr = new internal::gpubuffer(d_ptr, attr, size);
  MARE_DLOG("Creating mare::internal::gpubuffer %p", b_ptr);
  return mare::gpubuffer_ptr(b_ptr, gpubuffer_ptr::ref_policy::DO_INITIAL_REF);
}


}// ns mare

#endif //HAVE_OPENCL

