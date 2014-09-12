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
/** @file cofdeque.hh */
// TODO:
//    Add support for custom allocator
//
#pragma once

#include <mare/internal/cofdeque.hh>

namespace mare {

/** @addtogroup cof_deque
@{ */
/**
    Bounded double-ended queue, inspired by:

      Herlihy, Maurice, Victor Luchangco, and Mark
      Moir. "Obstruction-free synchronization: Double-ended queues as
      an example." Distributed Computing Systems,
      2003. Proceedings. 23rd International Conference on. IEEE, 2003.

      Herlihy, Maurice, Victor Luchangco, and Mark
      Moir. "Obstruction-free synchronization: Double-ended queues as
      an example." Distributed Computing Systems,
      2003. Proceedings. 23rd International Conference on. IEEE, 2003.

      Y. Afek and A. Morrison. Fast concurrent queues for x86
      processors. In PPoPP. ACM, 2013.

      H. Attiya, R. Guerraoui, D. Hendler, P. Kuznetsov, M. Michael,
      and M. Vechev. Laws of order: expensive synchronization in
      concurrent algorithms cannot be eliminated. In POP, 2011.

      G. Bar-Nissan, D. Hendler, and A. Suissa. A dynamic
      elimination-combining stack algorithm. In OPODIS, 2011.

      A. Haas, C. Kirsch, M. Lippautz, and H. Payer. How FIFO is your
      concurrent FIFO queue? In RACES. ACM, 2012.

      A. Haas, T. Henzinger, C. Kirsch, M. Lippautz, H. Payer,
      A. Sezgin, and A. Sokolova. Distributed queues in shared
      memory-multicore performance and scalability through
      quantitative relaxation. In Computing Frontiers. ACM, 2013.

      T. Harris. A pragmatic implementation of non-blocking
      linked-lists. In DISC. Springer, 2001.

      D. Hendler, N. Shavit, and L. Yerushalmi. A scalable lock-free
      stack algorithm. In SPAA. ACM, 2004.

      T. Henzinger, H. Payer, and A. Sezgin. Replacing competition
      with cooperation to achieve scalable lock-free FIFO
      queues. Technical Report IST-2013-124-v1+1, IST Austria, 2013.

      M. Hoffman, O. Shalev, and N. Shavit. The baskets queue. In
      OPODIS, pages 401-414. Springer, 2007.

      D. H. I. Incze, N. Shavit, and M. Tzafrir. Flat combining and
      the synchronization-parallelism tradeoff. In SPAA, pages
      355-364. ACM, 2010.

      M. Michael. CAS-based lock-free algorithm for shared deques. In
      Euro-Par. Springer, 2003.

      M. Michael and M. Scott. Simple, fast, and practical non-
      blocking and blocking concurrent queue algorithms. In PODC,
      pages 267-275. ACM, 1996.

      H. Sundell and P. Tsigas. Lock-free deques and doubly linked
      lists. Journal of Parallel and Distributed Computing, 68, 2008.

    The implementation is designed to work with values of any size; however,
    there is an optimized variant for values whose type is of a size
    less than or equal to that of the platform's size_t. This has the
    advantage of providing an optimized variant for native pointers.
*/

template<class T, class PREDICTOR = internal::cof::default_predictor>
class cof_deque {
public:
  typedef typename internal::cof::
    deque_size_t<T, PREDICTOR, sizeof(T) <= sizeof(size_t)> container_type;
  typedef T value_type;

  explicit cof_deque(size_t qsize) : _c(qsize) {}
  explicit cof_deque(cof_deque<T>& rhs) { *this = rhs; }

  /** Pushes value to the right of the queue.
      @return True if the push was successful; false if the queue was full.
   */
  bool right_push(const value_type& v) { return _c.right_push(v); }

  /** Pops from the right of the queue, placing the popped element in
      the results.

      @return True if the pop was successful; false if the queue was empty.
  */
  bool right_pop(value_type& r) { return _c.right_pop(r); }

  /** Pushes value to the left of the queue.

      @return True if the push was successful; false if the queue was full.
   */
  bool left_push(const value_type& v) { return _c.left_push(v); }
  /** Pops from the left of the queue, placing the popped element in
      the results.

      @return True if the pop was successful; false if the queue was empty.
  */
  bool left_pop(value_type& r) { return _c.left_pop(r); }

  /** Returns the size of the queue (number of elements it can accommodate). */
  size_t size() const { return _c.size(); }

private:
  container_type _c;
};

} //namespace mare
/** @} */ /* end_addtogroup cof_deque */
