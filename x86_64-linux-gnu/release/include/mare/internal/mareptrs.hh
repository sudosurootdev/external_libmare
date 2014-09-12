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

#include <atomic>
#include <memory>
#include <utility>

#include<mare/internal/debug.hh>
#include<mare/internal/log/log.hh>

// Hide warning from VS2012 about inline specifier used with a friend
// declaration.  We disabled some friend declarations in other files,
// since it was crashing VS2012 Nov CTP.  Perhaps this warning is not
// needed once the compiler is updated to support this properly.
MARE_MSC_IGNORE_BEGIN(4396);

namespace mare {
namespace internal {

// forward declarations
template<typename T>
class ref_counted_object;

template<typename T>
class mare_shared_ptr;

template<typename T>
class mare_unsafe_ptr;

template <typename T>
void explicit_unref(ref_counted_object<T>* obj);

template <typename T>
void explicit_ref(ref_counted_object<T>* obj);

enum ref_count_policy{
  increase,
  decrease,
  do_nothing
};

namespace log {

template<typename T> struct mareptr_helper;

template<>
struct mareptr_helper<task>{

  static void ref(task* t, size_t count) {
    log::log_event(log::events::task_reffed(t, count));
  }

  static void unref(task* t, size_t count) {
    log::log_event(log::events::task_unreffed(t, count));
  }
};


template<>
struct mareptr_helper<group>{

  static void ref(group* g, size_t count) {
    log::log_event(log::events::group_reffed(g, count));
  }

  static void unref(group* g, size_t count) {
    log::log_event(log::events::group_unreffed(g, count));
  }
};


template<typename T>
struct mareptr_helper {

  static void ref(T* o, size_t count) {
    log::log_event(log::events::object_reffed(o, count));
  }

  static void unref(T* o, size_t count) {
    log::log_event(log::events::object_reffed(o, count));
  }
};

}

template<typename T>
class ref_counted_object {
public:
  typedef size_t counter_type;

  virtual ~ref_counted_object(){}

  counter_type use_count() const {
    return _num_refs.load();
  }

protected:

  explicit ref_counted_object(counter_type initial_value)
    :_num_refs(initial_value) {
  }

  constexpr ref_counted_object()
    :_num_refs(0) {
  }

private:

  void unref() {
    auto new_num_refs = --_num_refs;

    log::mareptr_helper<T>::unref(static_cast<T*>(this), new_num_refs);

    typename T::should_delete_functor should_delete;
    if ( new_num_refs == 0)
      if(should_delete(static_cast<T*>(this)))
        delete this;
  }

  void ref() {
    auto new_num_refs = ++_num_refs;
    log::mareptr_helper<T>::ref(static_cast<T*>(this), new_num_refs);
  }

  friend class mare_shared_ptr<T>;
  friend void explicit_unref<>(ref_counted_object<T>* obj);
  friend void explicit_ref<>(ref_counted_object<T>* obj);

  std::atomic<counter_type> _num_refs;
};

template <typename T>
void explicit_unref(ref_counted_object<T>* obj) {
  obj->unref();
}

template <typename T>
void explicit_ref(ref_counted_object<T>* obj) {
  obj->ref();
}

template<typename T>
T* c_ptr(T* p) {
  return p;
}

template<typename T>
T* c_ptr(mare_unsafe_ptr<T>& t);

template<typename T>
T* c_ptr(mare_unsafe_ptr<T> const& t);

template<typename T>
bool operator==(mare_unsafe_ptr<T> const&, mare_unsafe_ptr<T> const&);

template<typename T>
bool operator!=(mare_unsafe_ptr<T> const&, mare_unsafe_ptr<T> const&);

template<typename T>
class mare_unsafe_ptr {
public:

  typedef T type;
  typedef T* pointer;

  enum {
    is_private = false
  };

  // Constructors
  constexpr mare_unsafe_ptr()
    :_target(nullptr) {
  }

  constexpr mare_unsafe_ptr(std::nullptr_t)
    :_target(nullptr) {
  }

  // Copy constructor.
  mare_unsafe_ptr(mare_unsafe_ptr const& other)
    :_target(other._target) {
    // No need to update number of sharers because all mare_unsafe_ptr count
    // as a single sharer.
  }

  // Move Constructors
  mare_unsafe_ptr(mare_unsafe_ptr&& other)
    : _target(other._target) {
    other._target = nullptr;
  }

  /// Destructor. When a mare_unsafe_ptr goes out of scope
  ~mare_unsafe_ptr()  {
  }

  //Assignment
  mare_unsafe_ptr& operator=(mare_unsafe_ptr const& other) {
    reset(other._target);
    return *this;
  }

  // Move Assignment
  mare_unsafe_ptr& operator=(mare_unsafe_ptr&& other)  {
    if (other._target == _target) {
      return *this;
    }
    reset(other._target);
    other.reset();
    return *this;
  }

  void reset(){
    reset(nullptr);
  }

  void swap(mare_unsafe_ptr& other)
  {
    std::swap(_target, other._target);
  }

  bool unique() const {
    return false;
  }

  // Operators
  explicit operator bool() const  {
    return target() != nullptr;
  }

private:

  explicit mare_unsafe_ptr(pointer ref)
    :_target(ref) {
  }

  pointer target() const  {
    return _target;
  }

  void reset(pointer other) {
    _target = other;
  }

  friend class mare_shared_ptr<T>;
  friend pointer c_ptr<>(mare_unsafe_ptr<T>& t);
  friend pointer c_ptr<>(mare_unsafe_ptr<T> const& t);

  friend bool operator==<>(mare_unsafe_ptr<T> const&,
                           mare_unsafe_ptr<T> const&);
  friend bool operator!=<>(mare_unsafe_ptr<T> const&,
                           mare_unsafe_ptr<T> const&);
  //friend void swap(mare_unsafe_ptr&, mare_unsafe_ptr&);

  pointer _target;
};

template <typename T>
T* c_ptr(mare_unsafe_ptr<T>& ptr) {
  return (ptr.target());
}

template <typename T>
T* c_ptr(mare_unsafe_ptr<T> const& ptr) {
  return (ptr.target());
}

template<typename T>
bool operator==(mare_unsafe_ptr<T> const& ptrA,
                mare_unsafe_ptr<T> const& ptrB)  {
  return ptrA.target()==ptrB.target();
}

template<typename T>
bool operator==(mare_unsafe_ptr<T> const& ptr, std::nullptr_t)  {
  return !ptr;
}

template<typename T>
bool operator==(std::nullptr_t, mare_unsafe_ptr<T> const& ptr)  {
  return !ptr;
}

template<typename T>
bool operator!=(mare_unsafe_ptr<T> const& ptrA,
                mare_unsafe_ptr<T> const& ptrB)  {
  return ptrA.target()!=ptrB.target();
}

template<typename T>
bool operator!=(mare_unsafe_ptr<T> const& ptr, std::nullptr_t)  {
  return static_cast<bool>(ptr);
}

template<typename T>
bool operator!=(std::nullptr_t, mare_unsafe_ptr<T> const& ptr)  {
  return static_cast<bool>(ptr);
}

// forward declarations

template <typename T>
T* c_ptr(mare_shared_ptr<T>& t);

template <typename T>
T* c_ptr(mare_shared_ptr<T>const& t);

template <typename T>
void reset_but_not_unref(mare_shared_ptr<T>& t);

template<typename T>
bool operator==(mare_shared_ptr<T> const&, mare_shared_ptr<T> const&);

template<typename T>
bool operator!=(mare_shared_ptr<T> const&, mare_shared_ptr<T> const&);

template<typename T>
class mare_shared_ptr {
public:

  typedef T type;
  typedef ref_counted_object<T>* pointer_base;
  typedef T* pointer;

  enum {
    is_private = false
  };

  enum class ref_policy {
    DO_INITIAL_REF,
      NO_INITIAL_REF
      };

  // Constructors
  constexpr mare_shared_ptr()
    :_target(nullptr) {
  }

  constexpr mare_shared_ptr(std::nullptr_t)
    :_target(nullptr) {
  }

  //Copy constructor
  mare_shared_ptr(mare_shared_ptr const& other)
    :_target(other._target) {
    if (_target)
      _target->ref();
  }

  // Move Constructors
  mare_shared_ptr(mare_shared_ptr&& other)
    : _target(other._target) {
    // we must not use reset_counter here because it would cause
    // the reference counter to go down
    other._target = nullptr;
  }

  /// Destructor. It will destroy the object if it is
  /// the last mare_shared_ptr pointing to it.
  ~mare_shared_ptr() {
    reset(nullptr);
  }

  //Assignment
  mare_shared_ptr& operator=(mare_shared_ptr const& other) {
    if (other._target  == _target)
      return *this;

    reset(other._target);
    return *this;
  }

  //Assignment
  mare_shared_ptr& operator=(std::nullptr_t) {
    if (_target  == nullptr)
      return *this;

    reset(nullptr);
    return *this;
  }

  // Move Assignment
  mare_shared_ptr& operator=(mare_shared_ptr&& other) {
    if (other._target == _target) {
      return *this;
    }
    unref();

    _target = other._target;
    other._target = nullptr;

    return *this;
  }

  void swap(mare_shared_ptr& other)
  {
    std::swap(_target, other._target);
  }

  mare_unsafe_ptr<T> get() {
    return mare_unsafe_ptr<T>(static_cast<T*>(target()));
  }

  void reset(){
    reset(nullptr);
  }

  void reset(pointer_base ref){
    unref();
    acquire(ref);
  }

  // Operators
  explicit operator bool() const  {
    return target() != nullptr;
  }

  size_t use_count() const {
    if (target())
      return _target->use_count();
    return 0;
  }

  bool unique() const {
    return use_count() == 1;
  }

  explicit mare_shared_ptr(pointer_base ref) :
    _target(ref) {
    if (_target)
      _target->ref();
  }

  mare_shared_ptr(pointer_base ref,  ref_policy policy)
    :_target(ref) {
    if (_target && policy == ref_policy::DO_INITIAL_REF)
      _target->ref();
  }

private:

  pointer_base target() const  {
    return _target;
  }

  void acquire(pointer_base other){
    _target = other;
    if (_target)
      _target->ref();
  }

  void unref() {
    if (_target)
      _target->unref();
  }

  friend pointer c_ptr<>(mare_shared_ptr<T>& t);
  friend pointer c_ptr<>(mare_shared_ptr<T>const& t);


  friend void reset_but_not_unref<>(mare_shared_ptr<T>& t);
  friend bool operator==<>(mare_shared_ptr<T> const&,
                           mare_shared_ptr<T> const&);
  friend bool operator!=<>(mare_shared_ptr<T> const&,
                           mare_shared_ptr<T> const&);

  pointer_base _target;
};

template <typename T>
T* c_ptr(mare_shared_ptr<T>& t) {
  return static_cast<T*>(t.target());
}

template <typename T>
T* c_ptr(mare_shared_ptr<T>const& t) {
  return static_cast<T*>(t.target());
}

template <typename T>
void reset_but_not_unref(mare_shared_ptr<T>& t) {
  t._target = nullptr;
}


template<typename T>
bool operator==(mare_shared_ptr<T> const& ptrA,
                mare_shared_ptr<T> const& ptrB) {
  return c_ptr(ptrA) == c_ptr(ptrB);
}

template<typename T>
bool operator==(mare_shared_ptr<T> const& ptr, std::nullptr_t) {
  return !ptr;
}

template<typename T>
bool operator==(std::nullptr_t, mare_shared_ptr<T> const& ptr) {
  return !ptr;
}

template<typename T>
bool operator!=(mare_shared_ptr<T> const& ptrA,
                mare_shared_ptr<T> const& ptrB) {
  return ptrA.get()!=ptrB.get();
}

template<typename T>
bool operator!=(mare_shared_ptr<T> const& ptr, std::nullptr_t) {
  return static_cast<bool>(ptr);
}

template<typename T>
bool operator!=(std::nullptr_t, mare_shared_ptr<T> const& ptr) {
  return static_cast<bool>(ptr);
}

//std::shared_ptr doesn't have a virtual destructor
//so this inheritance is safe as long as there are no data
//members in mare_buffer_ptr class, else will result in memory
//leak. this is also the source of warning.
// squelch GCC complaints about non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");

template<typename T>
class mare_buffer_ptr : public std::shared_ptr<T>
{

MARE_GCC_IGNORE_END("-Weffc++");

private:
  typedef typename T::Type ElementType;
public:
  constexpr mare_buffer_ptr() :  std::shared_ptr<T>() { }

  template< class Y >
  explicit mare_buffer_ptr( Y* ptr ) : std::shared_ptr<T>(ptr) { }

  template< class Y, class Deleter >
  mare_buffer_ptr( Y* ptr, Deleter d ) : std::shared_ptr<T>(ptr, d) { }

  template< class Y, class Deleter, class Alloc >
  mare_buffer_ptr( Y* ptr, Deleter d, Alloc alloc ) :
    std::shared_ptr<T>(ptr, d, alloc) { }

  constexpr mare_buffer_ptr( std::nullptr_t ptr) : std::shared_ptr<T>(ptr) { }

  template< class Deleter >
  mare_buffer_ptr( std::nullptr_t ptr, Deleter d ) :
    std::shared_ptr<T>( ptr, d) { }

  template< class Deleter, class Alloc >
  mare_buffer_ptr( std::nullptr_t ptr, Deleter d, Alloc alloc ) :
    std::shared_ptr<T>( ptr, d, alloc) { }

  template< class Y >
  mare_buffer_ptr( const std::shared_ptr<Y>& r, T *ptr ) :
    std::shared_ptr<T>(r, ptr) { }

  mare_buffer_ptr( const std::shared_ptr<T>& r ) : std::shared_ptr<T>(r) { }

  template< class Y >
  mare_buffer_ptr( const std::shared_ptr<Y>& r ) : std::shared_ptr<T>(r) { }

  mare_buffer_ptr( std::shared_ptr<T>&& r ) : std::shared_ptr<T>(r) { }

  template< class Y >
  mare_buffer_ptr( std::shared_ptr<Y>&& r ) : std::shared_ptr<T>(r) { }

  template< class Y >
  explicit mare_buffer_ptr( const std::weak_ptr<Y>& r ) :
    std::shared_ptr<T>(r) { }

  template< class Y >
  mare_buffer_ptr( std::auto_ptr<Y>&& r ) : std::shared_ptr<T>(r) { }

  template< class Y, class Deleter >
  mare_buffer_ptr( std::unique_ptr<Y,Deleter>&& r ) : std::shared_ptr<T>(r) { }

  ElementType& operator [] (size_t index)
  {
    T& mare_buffer = std::shared_ptr<T>::operator * ();
    return mare_buffer[index];
  }

  const ElementType& operator [] (size_t index) const
  {
    T& mare_buffer = std::shared_ptr<T>::operator * ();
    return mare_buffer[index];
  }

  T& operator * () const { return std::shared_ptr<T>::operator * (); }

  T* operator -> () const { return std::shared_ptr<T>::operator -> (); }
};

} //namespace internal::mare
} //namespace internal

MARE_MSC_IGNORE_END(4396);

