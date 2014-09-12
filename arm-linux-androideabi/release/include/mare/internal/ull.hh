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
#include <mare/internal/debug.hh>
#include <mare/common.hh>
#include <mare/exceptions.hh>

// Tell MSC that we acknowledge the new behaviour about constructing
// arrays
MARE_MSC_IGNORE_BEGIN(4351);

namespace mare { namespace internal
{

/// \brief Unrolled linked list
///
/// This template implements an unrolled linked list por pointers.
/// The idea of the class is that, in most situations, you just need
/// to store a few elements (INLINED_BUCKET_SIZE). In those cases, you
/// don't need to do extra allocation. If you store more than
/// INLINED_BUCKET_SIZE elements, then the data structure allocates
/// buckets of EXTERNAL_BUCKET_SIZE elements and creates a circular
/// double linked list with them.
///
/// Notes:
///   - The data structure is not thread-safe
///   - Can't delete elements
///   - Can't access individual elements.
///   - ull stands for unrolled_linked_list
///   - a new external bucket is not created until the previous one is full
///   - there can't be empty buckets

template<typename T,
         std::size_t InlinedBucketSize_,
         std::size_t ExternalBucketSize_>
class ull {
private:
  static constexpr std::size_t InlinedBucketLast_ = InlinedBucketSize_-1;
  static constexpr std::size_t InlinedBucketMaxSize_ = 256;
  static constexpr std::size_t InlinedBucketMinSize_ = 1;
  static constexpr std::size_t ExternalBucketMaxSize_ = 256;
  static constexpr std::size_t ExternalBucketMinSize_ = 2;

protected:

  template<std::size_t Size_>
  class bucket {
  public:

    static constexpr std::size_t size = Size_;

    bucket(T* first, T* second):
      _elems(),
      _prev(this),
      _next(this),
      _offset(2) {
      _elems[0] = first;
      _elems[1] = second;
    }

    bucket(T* t, bucket* prev, bucket* next):
      _elems(),
      _prev(prev),
      _next(next),
      _offset(1) {
      MARE_INTERNAL_ASSERT(prev,"invalid null pointer");
      MARE_INTERNAL_ASSERT(next,"invalid null pointer");
      _elems[0] = t;
    }

    ~bucket() {}

    MARE_DELETE_METHOD(bucket());
    MARE_DELETE_METHOD(bucket(bucket<Size_>const& other));
    MARE_DELETE_METHOD(bucket<Size_>& operator=(bucket<Size_>const& other));

    size_t count() const {
      return _offset;
    }

    void insert(T* t) {
      MARE_INTERNAL_ASSERT(_elems[_offset] == nullptr,
                           "Bucket position already taken");
      _elems[_offset] = t;
      _offset++;
    }

    bool full() const {
      return _elems[size-1] != nullptr;
    }

    bool empty() const {
      return _elems[0] == nullptr;
    }

    bucket<Size_>* get_prev() const {
      return _prev;
    }

    bucket<Size_>* get_next() const {
      return _next;
    }

    void set_prev(bucket<Size_>* prev) {
      _prev = prev;
    }

    void set_next(bucket<Size_>* next) {
      _next = next;
    }

    T* operator[] (const size_t index) {
      return _elems[index];
    }

  private:
    T* _elems[Size_];
    bucket<Size_>* _prev;
    bucket<Size_>* _next;
    std::size_t _offset;
  };// bucket

  typedef bucket<ExternalBucketSize_> bucket_t;

public:
  ull():
    _elems()
  {
    static_assert(InlinedBucketSize_ <= InlinedBucketMaxSize_,
                  "Maximum value for InlinedBucketSize_ is 256");

    static_assert(ExternalBucketSize_ <= ExternalBucketMaxSize_,
                  "Maximum value for InlinedBucketSize_ is 256");

    static_assert(InlinedBucketSize_ >= InlinedBucketMinSize_,
                  "Minimum value for InlinedBucketSize_ is 1");

    static_assert(ExternalBucketSize_ >= ExternalBucketMinSize_,
                  "Minimum value for ExternalBucketSize_ is 2");
  }

  ull(T* t):
    _elems()
  {
    static_assert(InlinedBucketSize_ <= InlinedBucketMaxSize_,
                  "Maximum value for InlinedBucketSize_ is 256");

    static_assert(ExternalBucketSize_ <= ExternalBucketMaxSize_,
                  "Maximum value for InlinedBucketSize_ is 256");

    static_assert(InlinedBucketSize_ >= InlinedBucketMinSize_,
                  "Minimum value for InlinedBucketSize_ is 1");

    static_assert(ExternalBucketSize_ >= ExternalBucketMinSize_,
                  "Minimum value for ExternalBucketSize_ is 2");

    MARE_INTERNAL_ASSERT(t, "Can't initialize ull with null pointer.");

    _elems[0] = t;
  }

  void reset()
  {
    if (!overflows()) {
      reset_inline();
      return;
    }

    auto first_bucket = get_first_overflow_bucket();
    MARE_INTERNAL_ASSERT(first_bucket, "null ptr to first_bucket");

    auto a_bucket = first_bucket;
    do {
      auto next  = a_bucket->get_next();
      delete a_bucket;
      a_bucket = next;
    } while (a_bucket != first_bucket);

    reset_inline();
  }

  ~ull() {
    MARE_INTERNAL_ASSERT(empty(), "Please reset ull before deleting it.");
  }

  /// @brief Inserts pointer in data structure
  ///
  /// Adds pointer to data structure. It does not
  /// check for duplicates.
  /// @param t pointer to add to data structure.
  void push_back(T* t) {
    MARE_INTERNAL_ASSERT(t != nullptr, "Can't add NULL pointer to ull");

    // If inlined bucket is not full, just find an empty spot and
    // insert it there We could play more games with the last bucket
    // and also use it as index into the array
    if (!inlined_bucket_full()) {
      size_t i = 0;
      while (i < InlinedBucketSize_ && _elems[i] != nullptr) i++;
      _elems[i] = t;
      return;
    }

    // If inlined bucket is full, check first whether the last item is
    // being reused as a pointer. If not, create first external bucket
    // and move pointer in last position in the inlined array to the
    // first position in the bucket
    if (!overflows()) {
      overflow(t);
      return;
    }

    //Insert in last overflow bucket if none is available, create a new
    //one and insert it there.
    auto first_bucket = get_first_overflow_bucket();
    auto last_bucket = first_bucket->get_prev();
    if (!last_bucket->full()) {
      last_bucket->insert(t);
      return;
    }

    auto new_bucket = new bucket<ExternalBucketSize_>(t, last_bucket,
                                                       first_bucket);
    first_bucket->set_prev(new_bucket);
    last_bucket->set_next(new_bucket);
  }

  /// @brief Executes method on each item in the ull
  template<typename F>
  void for_each(F f){

    // INTERNAL_BUCKET_SIZE can legitimately be zero, so we test it to
    // make sure the compilers do not complain
    MARE_GCC_IGNORE_BEGIN("-Wtype-limits");
    MARE_CLANG_IGNORE_BEGIN("-Wtautological-compare");
    for (size_t i = 0; i < InlinedBucketLast_ &&
           _elems[i] != nullptr; ++i) {
      f(_elems[i]);
    }
    MARE_CLANG_IGNORE_END("-Wtautological-compare");
    MARE_GCC_IGNORE_END("-Wtype-limits");

    if (!inlined_bucket_full())
      return;

    if (!overflows()) {
      f(get_last_inlined_pointer());
      return;
    }

    auto first_bucket = get_first_overflow_bucket();
    auto a_bucket = first_bucket;
    do{
      for (size_t i=0; i < ExternalBucketSize_ && (*a_bucket)[i] != nullptr;
           ++i)
        f((*a_bucket)[i]);
      a_bucket = a_bucket->get_next();
    } while (a_bucket != first_bucket);
  }

  /// @brief Returns number of elements in data structure.
  ///
  /// This is a slow method and it is only intended for debugging purposes.
  ///
  /// @return Number of elements in data structure.
  size_t size() {
    size_t i = 0;
    const auto f = [&i](T*){i++;};
    for_each(f);
    return i;
  }

  /// @brief Checks whether ull is empty.
  /// @return True if data structure is empty. False otherwise.
  bool empty() const{
    return _elems[0] == nullptr;
  }

  /// @brief Counts number of allocated buckets.
  /// Use it only for unit testing
  /// @return Number of allocated buckets.
  size_t get_num_buckets() const {
    if (!overflows())
      return 0;

    size_t num_buckets = 0;
    auto first_bucket = get_first_overflow_bucket();
    MARE_INTERNAL_ASSERT(first_bucket, "null overflow bucket");

    auto a_bucket = first_bucket;
    do {
      auto next  = a_bucket->get_next();
      num_buckets++;
      a_bucket = next;
    } while (a_bucket != first_bucket);

    return num_buckets;
  }

protected:

  void reset_inline() {
    for (size_t i = 0; i < InlinedBucketSize_; i++)
      _elems[i] = nullptr;
  }

  T* get_last_inlined_pointer() const {
    return _elems[InlinedBucketLast_];
  }

  void set_last_inlined_pointer(T* t) {
    _elems[InlinedBucketLast_] = t;
  }

  bool inlined_bucket_full() const {
    return get_last_inlined_pointer() != nullptr;
  }

  void overflow(T* t) {
    auto b = new bucket_t(get_last_inlined_pointer(), t);
    set_last_inlined_pointer(
      reinterpret_cast<T*>(reinterpret_cast<intptr_t>(b) | (1UL)));
  }

  bool overflows() const {
    return reinterpret_cast<bucket_t*>(
      reinterpret_cast<intptr_t>(get_last_inlined_pointer()) & (1UL)) !=
      nullptr;
  }

  bucket_t* get_first_overflow_bucket() const {
    return reinterpret_cast<bucket_t*>(
      reinterpret_cast<intptr_t>(get_last_inlined_pointer()) & (~1UL));
  }

  T* _elems[InlinedBucketSize_];

  MARE_DELETE_METHOD(ull(ull<T, InlinedBucketSize_, ExternalBucketSize_>
                         const& other));
  MARE_DELETE_METHOD(ull<T, InlinedBucketSize_, ExternalBucketSize_>&
                     operator=(ull<T, InlinedBucketSize_,
                               ExternalBucketSize_> const& other));

};

} }; // namespace mare::internal
MARE_MSC_IGNORE_END(4351);
