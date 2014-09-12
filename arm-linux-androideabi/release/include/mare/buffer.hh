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

/** @cond HIDDEN */
#ifdef HAVE_OPENCL
/** @endcond */

#include <mare/internal/buffer.hh>

namespace mare {

namespace internal {

// forwarding declaration
namespace test {
class buffer_tests;
} //namespace test

  // GPU devices on the platform.
  extern std::vector<mare::device_ptr> s_dev_ptrs;
  extern int s_page_size;
}

/** @addtogroup buffers_doc
@{ */
template<typename ElementType>
class buffer : public internal::buffer<ElementType>
{
public:
  /// Constructor.
  buffer(ElementType* ptr, size_t sz) : internal::
                                        buffer<ElementType>(ptr, sz) { }
  /// Constructor.
  buffer(size_t sz) : internal::buffer<ElementType>(sz) { }

  /// Copies the data back to backing store which can be either user
  /// provided or runtime allocated or it can be host-accessible device
  /// memory. If this method is called when the kernel is still running,
  /// then the calling thread is blocked until the kernel completes
  /// execution and data is copied back.
  void sync() { this->operator[](0); }

#ifdef ONLY_FOR_DOXYGEN
  /// Returns the size in elements of the buffer.
  /// @return Returns the size in elements of the buffer.
  inline size_t size();

  /// Access an element of buffer.
  /// If the buffer is used in a kernel launch, calling this function would
  /// block until the kernel completes and data is moved back to host
  /// backing store which can be either provided by
  /// the user during buffer creation or is created by MARE runtime or
  /// it simply refers to host-accessible device memory.
  ///
  /// @param index Index of the element to access.
  /// @return Returns the element stored at index.
  ElementType& operator[](size_t index);

  /// Returns pointer to the host backing store. This is equivalent
  /// to calling operator[] with index equals 0.
  ElementType* data();
#endif // ONLY_FOR_DOXYGEN
};

/**
  Creates a buffer of given size with no backing store on host.
  If this buffer is used on the host before being used on a device,
  host memory is allocated and used as the backing store, so in this case
  the buffer behaves as though it was created with user provided data.
  @sa mare::create_buffer(T* ptr, size_t size).

  If this buffer is used on a device and is never accessed on the host,
  no host memory will be allocated for the buffer. This may be useful
  in case of buffers used across consecutive kernel launches.

  If this buffer is used on a device and then accessed on the host, then
  no new host memory will be allocated since the buffer may already be
  allocated in host-accessible device memory when used on the device.

  @param size Size in number of elements of a given Type.

  @throws api_exception if size is 0.

  @par Example
  @includelineno create_buffer_without_host_ptr.cc

  @return
  mare_buffer_ptr<T> -- Buffer pointer that points to new buffer.
*/
template<typename T>
inline buffer_ptr<T>
create_buffer(size_t size)
{
  MARE_API_ASSERT(size, "buffer size can't be 0");
  typedef T ElementType;
  typedef internal::mare_buffer_ptr<mare::buffer<ElementType> > buffer_ptr;
  auto b_ptr = new buffer<ElementType>(size);
  MARE_DLOG("buffer: [%p] -- Created", b_ptr);
  return buffer_ptr(b_ptr);
}

/** @cond HIDDEN */
template<typename Collection>
struct buffer_traits
{
  typedef typename std::conditional<std::is_const<Collection>::value,
                           typename std::add_const<typename Collection::
                                                   value_type>::type,
                           typename Collection::value_type>::type ElementType;
};
/** @endcond **/

/**
  Creates a buffer of size elements with data provided in ptr
  as the backing store. Modifying the contents using ptr after
  this call is undefined, since it will be used as a backing store
  for syncing data to and from the device where this buffer is
  used.

  @param ptr Pointer to host data to be used as backing store.
  @param size  Size in number of elements, it's assumed that the
             application has allocated sz * sizeof(ElementType) bytes
             ptr is pointing to it.

  @throws api_exception if size is 0.

  @par Example
  @include snippets/create_buffer_with_host_ptr.cc

  @return
  mare_buffer_ptr<T> -- Buffer pointer that points to new buffer.
*/
template<typename T>
inline buffer_ptr<T>
create_buffer(T* ptr, size_t size)
{
  MARE_API_ASSERT((ptr != nullptr), "host ptr can't be null");
  MARE_API_ASSERT(size, "buffer size can't be 0");
  typedef T ElementType;
  typedef internal::mare_buffer_ptr<mare::buffer<ElementType> > buffer_ptr;
  auto b_ptr = new buffer<ElementType>(ptr, size);
  MARE_DLOG("buffer: [%p] -- Created", b_ptr);
  return buffer_ptr(b_ptr);
}

/** @} */ /* end_addtogroup buffers_doc */

}// ns mare.

/** @cond HIDDEN */
#endif // HAVE_OPENCL
/** @endcond **/
