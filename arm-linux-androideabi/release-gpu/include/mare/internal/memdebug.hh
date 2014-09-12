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

// Only enable memory debugging if we are in debug mode.
// Do not enable it on Android (it uses up too much memory)
// Do not enable on clang (too much GCC specific code)
//#define MARE_MEMORY_DEBUGGING

// This define will disable calls to free(), so that memory is never
//reused or corrupted by bugs
//#define MARE_MEMORY_DISABLE_FREE

// Define the amount of padding to allocate before and after and
// check to make sure it is left untouched. A default value is
// provided in memdebug.cc if MARE_MEMORY_DEBUGGING is enabled.
// #define MARE_MEMORY_PADDING 0

#ifdef MARE_MEMORY_DEBUGGING
#include <string>
size_t check_alloc_list (const void *ptr);
size_t check_free_list (const void *ptr);
std::string find_alloc_list (const void *ptr);
std::string find_free_list (const void *ptr);
void after_init_memory_objects (void);
void dump_memory_objects (bool all = false);
void* mare_malloc_internal(size_t bytes, const char *filename, int linenum,
                           const char *function);
void* mare_realloc_internal(void* ptr, size_t bytes, const char *filename,
                            int linenum, const char *function);
void mare_free_internal(const char *filename, int linenum,
                        const char *function);
void* __internal_new (size_t bytes, const char *filename, int linenum,
                      const char *function);
inline void* operator new   (size_t bytes, const char *filename, int linenum,
                             const char *function) {
  return __internal_new (bytes, filename, linenum, function);
}
inline void* operator new[] (size_t bytes, const char *filename, int linenum,
                             const char *function) {
  return __internal_new (bytes, filename, linenum, function);
}
void __internal_AssertPointer(const void *ptr, const char *ptrname,
                              const size_t ptrtypesize, const char *filename,
                              int linenum, const char *function, bool lookup,
                              const char *reason = NULL);

// Check the current 'this' object, as well as the supplied pointer,
// this is the preferred interface to use when possible Note that the
// *Size methods work out the size of the pointed to object and check
// this as well, but this is not possible with a forward defined class
#define MARE_ASSERT_OBJ_PTR(ptr, ...)                                   \
  __internal_AssertPointer(this, "this", -1,                            \
                           __FILE__, __LINE__, __FUNCTION__, true),     \
    __internal_AssertPointer(ptr, #ptr, -1,                             \
                             __FILE__, __LINE__, __FUNCTION__, true,    \
                             ## __VA_ARGS__)

#define MARE_ASSERT_OBJ_PTR_SIZE(ptr, ...) \
  __internal_AssertPointer(this, "this", sizeof(*this),                 \
                           __FILE__, __LINE__, __FUNCTION__, true),     \
    __internal_AssertPointer(ptr, #ptr, sizeof(*ptr), __FILE__, __LINE__, \
                             __FUNCTION__, true, ## __VA_ARGS__)

// Same as the above, except the pointer you have is actually
// inheriting from some base class, use this if the memory checker
// can't lookup your pointer
#define MARE_ASSERT_CAST_OBJ_PTR(ptr, ...)                           \
  __internal_AssertPointer(this, "this", -1,                         \
                           __FILE__, __LINE__, __FUNCTION__, true),  \
    __internal_AssertPointer(dynamic_cast<void*>(ptr), #ptr, -1,     \
                             __FILE__, __LINE__, __FUNCTION__, true, \
                             ## __VA_ARGS__)

#define MARE_ASSERT_CAST_OBJ_PTR_SIZE(ptr, ...)                         \
  __internal_AssertPointer(this, "this", sizeof(*this),                 \
                           __FILE__, __LINE__, __FUNCTION__, true),     \
    __internal_AssertPointer(dynamic_cast<void*>(ptr), #ptr,            \
                             sizeof(*dynamic_cast<void*>(ptr)),         \
                             __FILE__, __LINE__, __FUNCTION__, true,    \
                             ## __VA_ARGS__)

// Check just the supplied pointer
#define MARE_ASSERT_PTR(ptr, ...)                                  \
  __internal_AssertPointer(ptr, #ptr, -1,                          \
                           __FILE__, __LINE__, __FUNCTION__, true, \
                           ## __VA_ARGS__)

#define MARE_ASSERT_PTR_SIZE(ptr, ...)                                  \
  __internal_AssertPointer(ptr, #ptr, sizeof(*ptr),                     \
                           __FILE__, __LINE__, __FUNCTION__, true,      \
                           ## __VA_ARGS__)

// Check just the supplied pointer, and do not look it up, because it
// was allocated somewhere else
#define MARE_ASSERT_EXT_PTR(ptr, ...)                               \
  __internal_AssertPointer(ptr, #ptr, -1,                           \
                           __FILE__, __LINE__, __FUNCTION__, false, \
                           ## __VA_ARGS__)

#define mare_new new(__FILE__, __LINE__, __FUNCTION__)
#define mare_delete(ptr) if (ptr) { MARE_ASSERT_PTR(ptr); delete(ptr); }
#define mare_delete_array delete
#define mare_malloc(bytes) \
  mare_malloc_internal(bytes, __FILE__, __LINE__, __FUNCTION__)
#define mare_free(ptr) \
  mare_free_internal(ptr, __FILE__, __LINE__, __FUNCTION__)
#else
#define mare_new new
#define mare_delete(ptr) delete(ptr)
#define mare_delete_array delete
#define mare_malloc malloc
#define mare_realloc realloc
#define mare_free free
#define MARE_ASSERT_OBJ_PTR(ptr, ...)      do { } while(0)
#define MARE_ASSERT_OBJ_PTR_SIZE(ptr, ...) do { } while(0)
#define MARE_ASSERT_CAST_OBJ_PTR(ptr, ...) do { } while(0)
#define MARE_ASSERT_CAST_OBJ_PTR_SIZE(ptr, ...) do { } while(0)
#define MARE_ASSERT_PTR(ptr, ...)          do { } while(0)
#define MARE_ASSERT_PTR_SIZE(ptr, ...)     do { } while(0)
#define MARE_ASSERT_EXT_PTR(ptr, ...)      do { } while(0)
#endif // MARE_MEMORY_DEBUGGING

// This should not be enabled by default. It is a special debugging
// feature that causes the dual queue and hazard pointers to use
// malloc instead of the default operator new. It is not used to
// override all operator new/delete usage, just those where this macro
// is used.
#ifdef MARE_QUEUE_FORCE_MALLOC

#define MARE_OPERATOR_NEW_OVERRIDE                                      \
  void* operator new (std::size_t s) {                                  \
    void *mem = malloc(s);                                              \
    if (!mem) throw std::bad_alloc();                                   \
    return mem;                                                         \
  }                                                                     \
  void operator delete (void* p) { free(p); }

#else

#define MARE_OPERATOR_NEW_OVERRIDE

#endif // MARE_QUEUE_FORCE_MALLOC
