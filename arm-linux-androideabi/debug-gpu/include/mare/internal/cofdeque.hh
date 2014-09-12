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

#include <stdlib.h>
#include <mare/internal/alignedatomic.hh>
#include <mare/internal/alignedmalloc.hh>

namespace mare {

namespace internal {

namespace cof {

/// \brief Concurrent Obstruction-Free Deque
///
/// Inspired by:
///
///   Herlihy, Maurice, Victor Luchangco, and Mark
///   Moir. "Obstruction-free synchronization: Double-ended queues as
///   an example." Distributed Computing Systems,
///   2003. Proceedings. 23rd International Conference on. IEEE, 2003.
///
///   Y. Afek and A. Morrison. Fast concurrent queues for x86
///   processors. In PPoPP. ACM, 2013.
///
///   H. Attiya, R. Guerraoui, D. Hendler, P. Kuznetsov, M. Michael,
///   and M. Vechev. Laws of order: expensive synchronization in
///   concurrent algorithms cannot be eliminated. In POP, 2011.
///
///   G. Bar-Nissan, D. Hendler, and A. Suissa. A dynamic
///   elimination-combining stack algorithm. In OPODIS, 2011.
///
///   M. Gorelik and D. Hendler. Brief announcement: an asymmetric
///   flat-combining based queue algorithm. In PODC, pages 319-321,
///   2013.
///
/// The implementation is designed to work with values of any size but
/// there is an optimized variant for values who type is of size less
/// than or equal to that of the platforms size_t. This has the
/// advantage of providing an optimized variant for native pointers.
///
/// \todo:
/// \li add support for left and right hints
/// \li Add support for custom allocator in internal::deque_size_t.
///
///

// types of nodes in the queue
enum {
  LN    = 0,       // left null
  RN    = 1,       // right null
  DN    = 2,       // dummy null
  VALUE = 3,       // an actual value is stored at the node
};

// The queue is composed of nodes, which are formed from:
//
//  type    - describes if the node is LN, RN, DN, or a VALUE
//  counter - version counter used in the algorithm to avoid ABA issues
//  value   - pointer valid stored as user data within a node
//
struct node {
#if MARE_HAS_ATOMIC_DMWORD
  typedef mare_dmword_t node_t;

  node_t _type    : 4;  // null type
#if MARE_SIZEOF_MWORD == 8
  node_t _counter : 60; // version counter to avoid ABA
  node_t _value   : 64; // space for pointer to store in queue node
#elif MARE_SIZEOF_MWORD == 4
  node_t _counter : 28; // version counter to avoid ABA
  node_t _value   : 32; // space for pointer to store in queue node
#else // MARE_SIZEOF_MWORD
#error "unknown MARE_SIZEOF_MWORD"
#endif // MARE_SIZEOF_MWORD

#else // MARE_HAS_ATOMIC_DMWORD
#warning "deque requires Double Machine Word atomics"
#endif // MARE_HAS_ATOMIC_DMWORD

  node(node_t type, node_t counter, node_t value) :
    _type(type),
    _counter(counter),
    _value(value) {}

  node() : _type(), _counter(), _value() {}
  node(const node& rhs) : _type(), _counter(), _value() { *this = rhs; }

  node& operator=(const node& rhs) {
    _type    = rhs._type;
    _counter = rhs._counter;
    _value   = rhs._value;

    return *this;
  }

  bool operator== (const node &rhs) {
    return _type == rhs._type && _counter == rhs._counter &&
      _value == rhs._value;
  }
};

// method to calculate an index into _array % _size + 2
static inline size_t
array_index(size_t i, size_t size)
{
  return i % (size + 2);
}

// calculate middle of ring
static inline size_t
middle(size_t size)
{
  return (size + 2) / 2;
}

class simple_predictor {
  // Pretty much the brain dead predictor described in
  // "Obstruction-free synchronization: Double-ended queues as an
  // example.".  Will be eventually accurate, when executing in
  // isolation for long enough), which is required for
  // obstruction-free
public:
  simple_predictor(size_t) {}
  MARE_DELETE_METHOD(simple_predictor());
  MARE_DEFAULT_METHOD(~simple_predictor());
  size_t right(alignedatomic<node>* array, size_t size) {
    return right(array, size, size);
  }
  size_t right(alignedatomic<node>* array, size_t size, size_t start) {
    for (;;) {
      for (auto i = start + 2 + size; i >= start; i--) {
        auto idx = array_index(i, size);
        auto type = array[idx].load(std::memory_order_seq_cst)._type;
        if (type == RN || type == DN) {
          auto back = array_index(size + i + 1, size);
          if (array[back].load(std::memory_order_relaxed)._type != RN) {
            return idx;
          }
        }
      }
    }
    MARE_UNREACHABLE("right: never get here.");
  }

  size_t left(alignedatomic<node>* array, size_t size) {
    return left(array, size, 0);
  }
  size_t left(alignedatomic<node>* array, size_t size, size_t start) {
    for (;;) {
      for (auto i = start; i < start + size + 2; i++) {
        auto idx = array_index(i, size);
        auto type = array[idx].load(std::memory_order_seq_cst)._type;
        if (type == LN || type == DN) {
          auto next = array_index(i + 1, size);
          if (array[next].load(std::memory_order_relaxed)._type != LN) {
            return idx;
          }
        }
      }
    }
    MARE_UNREACHABLE("left: never get here.");
  }
};

class hinting_predictor {
    // This predictor simply remembers the last result and continues
  // from there
public:
  MARE_DELETE_METHOD(hinting_predictor());
  MARE_DEFAULT_METHOD(~hinting_predictor());
  hinting_predictor(size_t sz) :
    _right(sz),
    _left(0),
    _p(sz) {}

  size_t right(alignedatomic<node>* array, size_t size) {
    auto exp = _right.load();
    auto r = _p.right(array, size, array_index(exp + 1, size));
    // The predictor methods are required to be "eventually accurate".
    // The caller will eventually test our result and call us again
    // until accuracy is obtained.  It is for this reason that we use
    // the weak form since the loop is in the caller _and_ we could
    // get lucky even though we lost the assignment.
    //
    // It would be reasonable to use std::memory_order_acq_rel here,
    // however, I don't think it makes a difference on our current
    // platforms.
    //
    if (_right.compare_exchange_weak(exp, r))
      return r;
    return exp;
  }

  size_t left(alignedatomic<node>* array, size_t size) {
    auto exp = _left.load();
    auto l = _p.left(array, size, array_index(exp - 1, size));
    // see the right method for rational
    if (_left.compare_exchange_weak(exp, l))
      return l;
    return exp;
  }

private:
  std::atomic<size_t> _right;
  std::atomic<size_t> _left;
  simple_predictor _p;
};

typedef hinting_predictor default_predictor;

template <class PREDICTOR>
class deque {
private:
  PREDICTOR _predictor;

  // We use a double word CAS, allowing to store the null bits, the counter,
  // and the actual pointer being stored in the deque.
  // size of queue (note the user given size is always _size
  // and the actual internal queue size is _size + 2
  const size_t _size;

  // the queue backing store which needs to be allocated specially
  alignedatomic<node>* _array;

  size_t right_check_predictor(node *left, node *right)
  {
    for (;;) {
      size_t k = _predictor.right(_array, _size);

      *left  = _array[array_index(k + _size + 1, _size)].load(
          std::memory_order_seq_cst);
      *right = _array[k].load(std::memory_order_relaxed);

      if (right->_type == RN && left->_type != RN) {
        // correct predictor
        return k;
      }

      if (right->_type == DN && left->_type != RN && left->_type != DN) {
        // correct predictor, but no RNs so need to make one
        node left_desired(left->_type, left->_counter + 1, left->_value);
        node right_desired(RN, right->_counter + 1, 0);

        if (_array[array_index(k + _size + 1, _size)].compare_exchange_strong(
                *left, left_desired, std::memory_order_seq_cst)) {
          if (_array[k].compare_exchange_strong(*right, right_desired,
                                                std::memory_order_seq_cst)) {
            *left  = left_desired;
            *right = right_desired;
            return k;
          }
        }
      }
    }
    MARE_UNREACHABLE("function right_check_predictor: never get here.");
  }

  size_t left_check_predictor(node *left, node *right)
  {
    for (;;) {
      size_t k = _predictor.left(_array, _size);

      *left  = _array[k].load(std::memory_order_seq_cst);
      *right = _array[array_index(k+1, _size)].load(std::memory_order_relaxed);

      if (left->_type == LN && right->_type != LN) {
        // correct predictor
        return k;
      }

      if (left->_type == DN && right->_type != LN && right->_type != DN) {
        // correct predictor, but no LNs so need to make one
        node left_desired(LN, left->_counter + 1, 0);
        node right_desired(right->_type, right->_counter + 1, right->_value);

        if (_array[array_index(k + 1, _size)].compare_exchange_strong(
                *right, right_desired, std::memory_order_seq_cst)) {
          if (_array[k].compare_exchange_strong(*left, left_desired,
                                               std::memory_order_seq_cst)) {
            *left  = left_desired;
            *right = right_desired;
            return k;
          }
        }
      }
    }
    MARE_UNREACHABLE("function left_check_predictor: never get here.");
  }

public:
  deque(size_t qsize) :
    _predictor(qsize),
    _size(qsize),
    _array(nullptr) {

    auto sz = sizeof(*_array) * (_size + 2);
    auto mem = mare_aligned_malloc(MARE_SIZEOF_DMWORD, sz);
    if (mem == nullptr)
      throw std::bad_alloc();

    // You may argue that a "placement new" would be good here,
    // unfortunately a compiler implementation (like VS2012 in x64
    // mode) is allowed to add arbitrary bytes to the node class which
    // we need to be exact what is specified and "tight" so we will
    // cast it.
    memset(mem, 0, sz);
    _array = static_cast<alignedatomic<node>*>(mem);

    size_t middleish = middle(_size);

    for (size_t i = 0; i < _size + 2; i++) {
      node a;
      if (i < middleish) {
        a._type = LN;
      } else {
        a._type = RN;
      }
      a._counter = 0;
      a._value   = 0;
      _array[i].store(a);
    }

    mare_atomic_thread_fence(std::memory_order_seq_cst);
  }

  ~deque() {
    mare_aligned_free(_array);
  }

  MARE_DELETE_METHOD(deque());
  MARE_DELETE_METHOD(deque(const deque& rhs));
  MARE_DELETE_METHOD(deque& operator=(const deque&));

  size_t size() const { return _size; }

  // now the pop, push, and other queue related methods
  bool right_push(size_t value) {
    size_t k;
    node previous;
    node current;
    node next;
    node nextnext;

    for (;;) {
      k = right_check_predictor(&previous, &current);

      next = _array[array_index(k+1, _size)].load(std::memory_order_seq_cst);

      if (next._type == RN) {
        if (_array[array_index(k + _size + 1, _size)].compare_exchange_strong(
                previous,
                node(previous._type, previous._counter + 1, previous._value),
                std::memory_order_seq_cst)) {
          if (_array[k].compare_exchange_strong(
                  current,
                  node(VALUE, current._counter+1, value),
                  std::memory_order_seq_cst)) {

            return true;
          }
        }
      }

      if (next._type == LN) {
        if (_array[k].compare_exchange_strong(
                current,
                node(RN, current._counter + 1, 0),
                std::memory_order_seq_cst)) {
          // LN -> DN
          _array[array_index(k+1, _size)].compare_exchange_strong(
              next,
              node(DN, next._counter + 1, 0),
              std::memory_order_seq_cst);
        }
      }

      if (next._type == DN) {
        nextnext =
          _array[array_index(k + 2, _size)].load(std::memory_order_seq_cst);

        if (nextnext._type == VALUE) {
          if (_array[array_index(k + _size + 1, _size)].load(
                  std::memory_order_seq_cst) == previous) {
            if (_array[k].load(std::memory_order_seq_cst) == current) {
              return false;
            }
          }
        }

        if (nextnext._type == LN) {
          if (_array[array_index(k+2, _size)].compare_exchange_strong(
                  nextnext,
                  node(nextnext._type, nextnext._counter + 1,nextnext._value),
                  std::memory_order_seq_cst)) {
            // DN -> RN
            _array[array_index(k+1, _size)].compare_exchange_strong(
                next,
                node(RN, next._counter + 1, 0),
                std::memory_order_seq_cst);
          }
        }
      }
    }
  }

  bool right_pop(size_t* result)
  {
    size_t k;
    node current;
    node next;

    for (;;) {
      k = right_check_predictor(&current, &next);

      if ((current._type == LN || current._type == DN) &&
          _array[array_index(k + _size + 1, _size)].load(
              std::memory_order_seq_cst) == current) {
        return false;
      }

      if (_array[k].compare_exchange_strong(
              next,
              node(RN, next._counter + 1, 0),
              std::memory_order_seq_cst)) {
        if (_array[array_index(k + _size + 1, _size)].
            compare_exchange_strong(
                current,
                node(RN, current._counter + 1, 0),
                std::memory_order_seq_cst)) {
          *result = current._value;
          return true;
        }
      }
    }
  }

  bool left_push(size_t value)
  {
    size_t k;
    node previous;
    node current;
    node next;
    node nextnext;

    for (;;) {
      k = left_check_predictor(&current, &previous);

      next = _array[array_index(k + _size + 1, _size)].load(
          std::memory_order_seq_cst);

      if (next._type == LN) {
        // LN -> VALUE
        if (_array[array_index(k + 1, _size)].compare_exchange_strong(
                previous,
                node(previous._type, previous._counter + 1, previous._value),
                std::memory_order_seq_cst)) {
          if (_array[k].compare_exchange_strong(
                  current,
                  node(VALUE, current._counter + 1, value),
                  std::memory_order_seq_cst)) {
            return true;
          }
        }
      }

      if (next._type == RN) {
        if (_array[k].compare_exchange_strong(
                current,
                node(LN,current._counter + 1, 0),
                std::memory_order_seq_cst)) {
          // RN -> DN
          _array[array_index(k + _size + 1, _size)].
            compare_exchange_strong(next,
                                    node(DN,next._counter + 1, 0),
                                    std::memory_order_seq_cst);
        }
      }

      if (next._type == DN) {
        nextnext = _array[array_index(k + _size, _size)].load(
            std::memory_order_seq_cst);

        if (nextnext._type == VALUE) {
          if(_array[array_index(k+1, _size)].load(
                 std::memory_order_seq_cst) == previous) {
            if (_array[k].load(std::memory_order_seq_cst) == current) {
              return false;
            }
          }
        }

        if (nextnext._type == RN) {
          if (_array[array_index(k + _size, _size)].compare_exchange_strong(
                  nextnext,
                  node(nextnext._type, nextnext._counter + 1, nextnext._value),
                  std::memory_order_seq_cst)) {
            // DN -> LN
            if (_array[array_index(k + _size + 1, _size)].
                compare_exchange_strong(next,
                                        node(LN, next._counter + 1, 0),
                                        std::memory_order_seq_cst)) {
            }
          }
        }
      }
    }
  }

  bool left_pop(size_t* result)
  {
    size_t k;
    node current;
    node next;

    for (;;) {
      k = left_check_predictor(&next, &current);

      if ((current._type == RN || current._type == DN) &&
          _array[array_index(k+1, _size)].load(
              std::memory_order_seq_cst) == current) {
        return false;
      }

      if (_array[k].compare_exchange_strong(
              next,
              node(LN, next._counter + 1, 0),
              std::memory_order_seq_cst)) {
        if (_array[array_index(k+1, _size)].compare_exchange_strong(
                current,
                node(LN, current._counter + 1, 0),
                std::memory_order_seq_cst)) {
          *result = current._value;
          return true;
        }
      }
    }

  }
}; // class deque

//
// We use template specialization to provide two implementations of
// the underlying deque. One is a generic implementation that works
// for any type, but requires the use of an allocator to reserve
// storage to store the value while stored in the deque. The second is
// an optimized instance for values that have storage requirements less
// than or equal to that of a size_t.
//
template<class T, class PREDICTOR, bool FITS_IN_SIZE_T>
class deque_size_t
{
  // true and false specializations given below
};

// Specialize the queue for the case when the sizeof(T) > sizeof(size_t)
template<class T, class PREDICTOR>
class deque_size_t<T, PREDICTOR, false>
{
private:
  deque<PREDICTOR> _deque;
public:
  deque_size_t(size_t qsize) : _deque(qsize) { }

  bool right_push(T& value)
  {
    // allocated space to store value
    T * ptr = new T;
    *ptr = value;

    bool res = _deque.right_push(reinterpret_cast<size_t>(ptr));
    if (res)
      return true;

    delete ptr;
    return false;
  }

  deque_size_t(const deque_size_t<T, PREDICTOR, false>& rhs) { *this = rhs; }

  //
  // Only shallow copy is provided
  //
  deque_size_t<T, PREDICTOR, false>& operator= (
      const deque_size_t<T, PREDICTOR, false>& rhs) {
    _deque = rhs._deque;

    return *this;
  }

  bool right_pop(T* result) {
    T* ptr;
    bool res = _deque.right_pop(static_cast<size_t*>(&ptr));
    if (!res)
      return false;

    *result = *ptr;

    // delete the associated backing memory
    delete ptr;

    return true;
  }

  bool left_push(T& value) {
    // allocated space to store value
    T* ptr = new T;
    *ptr = value;

    bool res = _deque.left_push(reinterpret_cast<size_t>(ptr));
    if (!res)
      delete ptr;

    return res;
  }

  bool left_pop(T* result) {
    T* ptr;
    bool res = _deque.left_pop(static_cast<size_t*>(&ptr));
    if (!res)
      return false;

    *result = *ptr;

    // delete the associated backing memory
    delete ptr;

    return true;
  }

  size_t size() const { return _deque.size(); }
};

template<class T, class U>
struct deque_convert
{
  static U cast_from(T v) { return static_cast<U>(v); }

  static T cast_to(U v) { return static_cast<T>(v); }
};

template<class T, class U>
struct deque_convert<T*, U>
{
  static U cast_from(T* v) { return reinterpret_cast<U>(v); }

  static T* cast_to(U v) { return reinterpret_cast<T*>(v); }
};

// Specialize the queue for the case when the sizeof(T) <= sizeof(size_t)
template<class T, class PREDICTOR>
class deque_size_t<T, PREDICTOR, true>
{
public:
  typedef T value_type;
  deque_size_t<T, PREDICTOR, true>(size_t qsize) : _deque(qsize) {}
  deque_size_t<T, PREDICTOR, true>(
      const deque_size_t<T, PREDICTOR, true>& rhs) {
    *this = rhs;
  }

  deque_size_t<T, PREDICTOR, true>& operator= (
      const deque_size_t<T, PREDICTOR, true>& rhs) {
    _deque = rhs._deque;
    return *this;
  }

  bool right_push(const value_type& value) {
    return _deque.right_push(
        deque_convert<value_type, size_t>::cast_from(value));
  }

  bool right_pop(value_type& result) {
    size_t tmp;
    bool res = _deque.right_pop(&tmp);
    if (res)
      result = deque_convert<value_type, size_t>::cast_to(tmp);

    return res;
  }

  bool left_push(const value_type& value) {
    return _deque.left_push(
        deque_convert<value_type, size_t>::cast_from(value));
  }

  bool left_pop(value_type& result) {
    size_t tmp;
    bool res = _deque.left_pop(&tmp);
    if (res)
      result = deque_convert<value_type, size_t>::cast_to(tmp);
    return res;
  }
  size_t size() const { return _deque.size(); }

private:
  deque<PREDICTOR> _deque;
};

}; // namespace cof

}; // namespace internal

}; // namespace mare
