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

#ifdef HAVE_OPENCL

#include <mutex>

#include <mare/attr.hh>
#include <mare/device.hh>

#include <mare/internal/alignedmalloc.hh>
#include <mare/internal/attr.hh>
#include <mare/internal/debug.hh>
#include <mare/internal/gpubuffer.hh>
#include <mare/internal/pagefault.hh>

namespace mare {

namespace internal {

// forwarding declaration
namespace test {
class buffer_tests;
} //namespace test

// GPU devices on the platform.
extern std::vector<mare::device_ptr> s_dev_ptrs;
extern int s_page_size;

//buffer class is not thread safe.
/**
  Buffer provides an abstraction of shared virtual memory(SVM)
  to the user.
*/
template<typename ElementType>
class buffer : public internal::protected_buffer
{
private:
  ElementType* _host_data;
  size_t _size;
  gpubuffer_ptr _gpu_buffer;
  device_ptr _device;
  buffer_state _cur_state;
  buffer_state _start_state;
  bool _runtime_allocated;
  //Default, set to INOUT.
  buffer_usage _cur_usage;

  MARE_DELETE_METHOD(buffer(buffer const&));
  MARE_DELETE_METHOD(buffer(buffer&&));
  MARE_DELETE_METHOD(buffer& operator=(buffer const&));
  MARE_DELETE_METHOD(buffer& operator=(buffer&&));

  void copy_from_device(device_ptr const& device);

  inline void set_clean() {
    if(_cur_state == buffer_state::CLEAN)
      return;

    _cur_state = buffer_state::CLEAN;
    internal::pagefault::protect_buffer(*this);
    MARE_DLOG("buffer: [%p] -- State: CLEAN", this);
  }

public:
  inline void set_invalid() {
    if(_cur_state == buffer_state::INVALID)
      return;

    //Buffer in INIT state is never protected.
    if(_start_state != buffer_state::INIT)
      internal::pagefault::unprotect_buffer(*this);

    _cur_state = buffer_state::INVALID;
    MARE_DLOG("buffer: [%p] -- State: INVALID", this);
  }

  inline void set_current_usage(buffer_usage usage) {
    _cur_usage = usage;
  }
  typedef ElementType Type;

  //Implemenation notes.
  //creates a cl::Buffer with USE_HOST_PTR(UHP) flags when used as a
  //parameter to create_ndrange_task or pfor_each.
  //Initial state is DIRTY since the GPU doesn't have an updated copy
  //of the buffer, all DIRTY buffers used in kernel launch will be
  //copied automatically.
  buffer(ElementType* ptr, size_t sz) : _host_data(ptr),
                                        _size(sz),
                                        _gpu_buffer(nullptr),
                                        _device(internal::s_dev_ptrs[0]),
                                        _cur_state(buffer_state::DIRTY),
                                        _start_state(buffer_state::DIRTY),
                                        _runtime_allocated(false),
                                        _cur_usage(buffer_usage::INOUT) {
    MARE_DLOG("buffer: [%p] host_ptr: [%p] -- State: DIRTY, cur_usage: INOUT",
              this, ptr);
  }

  //Implemenation notes.
  //creates a cl::Buffer as follows:
  //1) if mare::buffer operator [] is called before being used on the gpu
  //   then it uses USE_HOST_PTR(UHP) flag during cl::Buffer creation.
  //2) else it uses ALLOC_HOST_PTR(AHP) flag during cl::Buffer creation.
  buffer(size_t sz) : _host_data(nullptr),
                      _size(sz),
                      _gpu_buffer(nullptr),
                      _device(internal::s_dev_ptrs[0]),
                      _cur_state(buffer_state::INIT),
                      _start_state(buffer_state::INIT),
                      _runtime_allocated(false),
                      _cur_usage(buffer_usage::INOUT) {
    MARE_DLOG("buffer: [%p] -- State: INIT, cur_usage: INOUT", this);
  }

  /// Sets the current usage of the buffer as IN only, this
  /// avoids copy back from device when the buffer is accessed on host.
  inline void set_current_usage_in() {
    set_current_usage(buffer_usage::IN);
    MARE_DLOG("buffer: [%p] -- cur_usage: IN", this);
  }

  /// Sets the current usage of the buffer as OUT only, this
  /// avoids copy to device when the buffer is accessed on a device.
  inline void set_current_usage_out() {
    set_current_usage(buffer_usage::OUT);
    MARE_DLOG("buffer: [%p] -- cur_usage: OUT", this);
  }

  /// Sets the current usage of the buffer as INOUT, this involves
  /// copy to device when the buffer is accessed on a device and copy
  /// back to host when it's accessed on host.
  inline void set_current_usage_inout() {
    set_current_usage(buffer_usage::INOUT);
    MARE_DLOG("buffer: [%p] -- cur_usage: INOUT", this);
  }

  /// Initializes the gpu buffer for a given buffer.
  void init_gpu_buffer(device_ptr const& device);

  const void* begin() {
    return data();
  }

  const void* end() {
    return static_cast<const void*>(data() + size());
  }

  void set_dirty() {
    MARE_INTERNAL_ASSERT(_cur_state != buffer_state::INIT,
                         "buffer: [%p] -- State: %d,INIT -> DIRTY invalid",
                         this, _cur_state);
    if(_cur_state == buffer_state::DIRTY)
      return;

    _cur_state = buffer_state::DIRTY;
    internal::pagefault::unprotect_buffer(*this);
    MARE_DLOG("buffer: [%p] -- State: DIRTY", this);
  }

  inline size_t num_pages() {
    return (_size*sizeof(ElementType)+internal::s_page_size-1) /
           internal::s_page_size;
  };

  inline size_t size() {
    return _size;
  }

  gpubuffer_ptr& get_gpu_buffer() {
    return _gpu_buffer;
  }

  device_ptr& get_device() {
    return _device;
  }

  ElementType& operator[](size_t index) {
    ElementType* ptr = data();
    return ptr[index];
  }

  ElementType* data();

  void copy_to_device(device_ptr const& device);

  ~buffer() {
    if(_start_state != buffer_state::INIT)
      internal::pagefault::unregister_buffer(*this);
    //free only if runtime allocated the host_ptr.
    if(_runtime_allocated) {
      internal::mare_aligned_free(const_cast<typename std::add_pointer<
                                  typename std::remove_const<
                                  ElementType>::type>::type>(_host_data));
    }
  }

  struct should_delete_functor {
    bool operator()(buffer<ElementType>* b) {

      MARE_API_DETAILED_ASSERT((b != nullptr), "null buffer pointer");
      MARE_API_DETAILED_ASSERT(b->use_count() == 0,
                               "Deleter called before counter hit 0.");
      return true;
    }
   }; // should_delete_functor
  friend class internal::test::buffer_tests;
};

template<typename ElementType>
void
buffer<ElementType>::init_gpu_buffer(device_ptr const& device)
{
  if(_gpu_buffer != nullptr)
    return;

  typedef typename std::conditional<std::is_const<ElementType>::value,
                                   decltype(mare::buffer_attr::read_only),
                                   decltype(mare::buffer_attr::read_write)>::
                                   type BufferAttribute;
  BufferAttribute battr;
  size_t len = _size*sizeof(ElementType);
  if(_host_data)
    _gpu_buffer = ::mare::create_buffer(device, battr, _host_data, len);
  else
    _gpu_buffer = ::mare::create_buffer(device, battr, len);
}

//synchronizes host data with a device buffers.
template<typename ElementType>
void
buffer<ElementType>::copy_from_device(device_ptr const& device)
{
  init_gpu_buffer(device);
  auto d_ptr = internal::c_ptr(device);
  MARE_API_ASSERT((d_ptr != nullptr), "null device_ptr");
  auto b_ptr = internal::c_ptr(_gpu_buffer);

  //If a buffer is being used as a IN buffer, there is no need for copy-out.
  if(_cur_usage != buffer_usage::IN) {
    //For AHP buffers - _host_data will be address from clenqueueMapBuffer.
    //For UHP buffers - _host_data is same as _host_ptr supplied from user.
    _host_data = static_cast<ElementType*>(b_ptr->copy_from_device());
  }

  MARE_DLOG("buffer: [%p] -- Copy_from_device to: %p", this, _host_data);
  if(_start_state == buffer_state::INIT ||
     std::is_const<ElementType>::value)
    _cur_state = buffer_state::DIRTY;
  else
    set_clean();
}

template<typename ElementType>
ElementType*
buffer<ElementType>::data()
{
  switch(_cur_state) {
    case buffer_state::INIT:
      //Buffer has been created without _host_data
      //and being accessed on host.
      if((_host_data == nullptr) && (_gpu_buffer == nullptr)) {
        size_t sz = num_pages()*internal::s_page_size;
        _host_data = reinterpret_cast<ElementType*>(
                     internal::mare_aligned_malloc(internal::s_page_size, sz));
        MARE_INTERNAL_ASSERT((_host_data != nullptr),
                             "aligned_alloc returned null");
        if (_host_data == nullptr) {
         abort();
        }
        MARE_DLOG("buffer: [%p] -- Allocated _host_data: %p",this,
                  _host_data);
        _runtime_allocated = true;
        //This case is equivalent to creating a buffer in CLEAN state.
        //So the _start_state needs to be updated.
        _start_state = buffer_state::CLEAN;
        set_clean();
      }
      break;

    case buffer_state::CLEAN:
    case buffer_state::DIRTY:
      break;

    case buffer_state::INVALID:
      copy_from_device(_device);
      break;

    default:
      MARE_API_ASSERT(false, "buffer: [%p] -- Unknown buffer_state", this);
      break;
  }
  MARE_API_ASSERT((_host_data != nullptr),
                  "buffer: [%p] -- null ptr from data()", this);
  return _host_data;
}

//copies data to the device.
template<typename ElementType>
void
buffer<ElementType>::copy_to_device(device_ptr const& device)
{
  init_gpu_buffer(device);
  MARE_API_ASSERT((_cur_usage != buffer_usage::OUT),
                  "copy_to_device called on a buffer with OUT usage");
  auto d_ptr = internal::c_ptr(device);
  MARE_API_ASSERT((d_ptr != nullptr), "null device_ptr");
  auto b_ptr = internal::c_ptr(_gpu_buffer);

  //AHP buffers need to be unmapped before it can
  //be used on the kernel, irrespective of the current
  //state of mare::buffer.
  if(_start_state == buffer_state::INIT) {
    b_ptr->copy_to_device();
    _host_data = nullptr;
  }

  //UHP buffers needs to be copied only if the
  //current state is dirty
  else if(_cur_state == buffer_state::DIRTY) {
    b_ptr->copy_to_device();
  }
  //Copy to device happens automatically during kernel launch.
  //After a buffer is copied to the device, the kernel may
  //modify it, so the state is set to INVALID indicating that
  //the host copy is stale. Any access of an invalid buffer
  //on host will result in copy from device.
  set_invalid();
}

template<typename T>
inline internal::mare_buffer_ptr<buffer<T> >
create_buffer(size_t size)
{
  MARE_API_ASSERT(size, "buffer size can't be 0");
  typedef T ElementType;
  typedef internal::mare_buffer_ptr<mare::buffer<ElementType> > buffer_ptr;
  auto b_ptr = new buffer<ElementType>(size);
  MARE_DLOG("buffer: [%p] -- Created", b_ptr);
  return buffer_ptr(b_ptr);
}

template<typename Collection>
struct buffer_traits
{
  typedef typename std::conditional<std::is_const<Collection>::value,
                           typename std::add_const<typename Collection::
                                                   value_type>::type,
                           typename Collection::value_type>::type ElementType;
};

template<typename T>
inline internal::mare_buffer_ptr<buffer<T> >
create_buffer(T* ptr, size_t size)
{
  MARE_API_ASSERT((ptr != nullptr), "host ptr can't be null");
  MARE_API_ASSERT(size, "buffer size can't be 0");
  typedef T ElementType;
  typedef internal::mare_buffer_ptr<mare::buffer<ElementType> > buffer_ptr;
  auto b_ptr = new buffer<ElementType>(ptr, size);
  MARE_DLOG("buffer: [%p] hostptr: [%p] -- Created", b_ptr, ptr);
  return buffer_ptr(b_ptr);
}

}// ns internal.
}// ns mare.
#endif // HAVE_OPENCL
