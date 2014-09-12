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
/** @file pagefault.hh */
#pragma once

#ifdef HAVE_OPENCL

#include <cstddef>

#include <mare/common.hh>

namespace mare {
namespace internal {

extern int s_page_size;

struct protected_buffer : public internal::ref_counted_object<protected_buffer>
{
  //INIT - Initial state of buffer created with only size.
  //CLEAN - Initial state of buffer created with host ptr and size.
  //INVALID - state of buffer after kernel launch.
  //DIRTY - state of buffer which is modified.
  //Transition between states is not thread-safe,
  enum class buffer_state {INIT, CLEAN, INVALID, DIRTY};
  enum class buffer_usage {IN, OUT, INOUT};
  virtual const void* begin() = 0;
  virtual const void* end() = 0;
  virtual size_t num_pages();
  virtual void set_dirty() = 0;
};

inline size_t protected_buffer::num_pages()
{
  std::ptrdiff_t c = static_cast<const char*>(end()) -
                     static_cast<const char*>(begin());
  return (c+internal::s_page_size-1)/internal::s_page_size;
}

namespace pagefault {

void protect_buffer(protected_buffer&);
void unprotect_buffer(protected_buffer&);
void unregister_buffer(protected_buffer&);
void install_handler();

}; // namespace pagefault
}; // namespace internal
}; // namespace mare
#endif // HAVE_OPENCL

