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

#include <mare/internal/mutex_cv.hh>
#include <mare/internal/alignedatomic.hh>
#include <mare/internal/hazardpointers.hh>
#include <mare/internal/debug.hh>
#include <iostream>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <stdlib.h>
#include <atomic>

//#ifndef NDEBUG
#include <unordered_set>
//#define MARE_DQ_DEBUG
//#define MARE_DQ_DEBUG_ELEMENTS
//#define MARE_DQ_DEBUG_MEMORY
//#endif

#ifdef MARE_DQ_DEBUG_ELEMENTS
#define MARE_DQ_DEBUG
#endif

#define MARE_CPTR_DELETEME_PTR (reinterpret_cast<qnode*>(0x1))
#define MARE_CPTR_CLAIMDELETION_PTR (reinterpret_cast<qnode*>(0x2))

namespace mare
{

namespace internal
{
///
/// Implementation of the Dualqueue:
/// http://www.cs.rochester.edu/research/synchronization/pseudocode/duals.html
///
/// Differences:
///   - use hazard pointers for memory management
///   - CAS-1-2 marking of nodes for freeing. A dequeue is allowed to bailout
///     prematurely, however at this leaves a node in the chain. We mark this
///     node as safe to delete and let the visiting enqueuer delete it.
///
/// IMPORTANT NOTE:
///   - When using a lock-based alignedatomic<T>, the unsafe delete is in fact
///     unsafe, because of the lock that is freed. Scenario: thread 1 locks,
///     memory is freed, thread 2 allocates memory in same spot and locks,
///     thread 1 tries to unlock.
///
template <typename T>
class DualTaskQueue {
public:
  struct cptr;
  struct ctptr;
  struct qnode;
  typedef hp::Record<qnode,2> HPRecord;

public:

  // Creates a new queue instance.
  DualTaskQueue():
#ifdef MARE_DQ_DEBUG
    _count(0),
    _waiting(0),
    _count_push_fastPass(0),
    _count_push_slowPass(0),
    _count_pop_fastPass(0),
    _count_pop_slowPass(0),
#endif
    _head(),
    _tail(),
    _mutex(),
    _HPManager()
  {

    // Setup sentinel qnode
    qnode* qn = new qnode();
    MARE_INTERNAL_ASSERT(qn, "Node is null");

    head().storeNA(cptr(qn));
    tail().storeNA(ctptr(qn,false));
#ifndef NDEBUG
    // to avoid false asserts on the first sentinel node
    qn->_dataObtainedFast = true;
#endif
  }

  ///
  /// Destroys the queue, frees any node still in the queue.
  /// NOTE: this should only be called if NO thread is using
  ///       the queue anymore.
  ///
  virtual ~DualTaskQueue() {
    qnode* node = head().load(std::memory_order_relaxed).ptr();
    while(node) {
      qnode* delNode = node;
      node = node->_next.load(std::memory_order_relaxed).ptr();
      delete delNode;
    }
  }

  ///
  /// Enqueue the specified element.
  /// Does not block.
  /// @param element The element to enqueue.
  ///
  void push(T element, bool do_notify = true) {

    // Setup new qnode
    qnode* node = new qnode(element);
    MARE_INTERNAL_ASSERT(node, "Node is null");
    MARE_INTERNAL_ASSERT(
      node->_next.load(std::memory_order_relaxed).ptr()==nullptr,
      "node init failure");
    MARE_INTERNAL_ASSERT(
      node->_request.load(std::memory_order_relaxed).ptr()==nullptr,
      "node init failure");

#ifndef NDEBUG
    typename hp::Manager<qnode,2>::CheckCleanOnDestroy checkClean(_HPManager);
#endif

    while (1) {
      ctptr myTail = tail().load(std::memory_order_relaxed);
      cptr myHead = head().load(std::memory_order_relaxed);

      // Check the state of the queue
      if ((myTail.ptr() == myHead.ptr()) || !myTail.isRequest()) {
        // queue empty, tail falling behind, or queue contains data
        // (queue could also contain exactly one outstanding request
        // with tail pointer as yet unswung)

        // Mark myTail as hazardous
        _HPManager.setMySlot(0,myTail.ptr());

        if(myTail != tail().load(std::memory_order_relaxed)) continue;

        // If the head changed, we might have wrongly determined that the
        // queue is empty, so start over. This could happen if myHead we read
        // at the time was deleted and the new tail was allocated in the old
        // location of the head. If we then read the tail, we will see that
        // myTail.ptr() == myHead.ptr() is true, but the queue could contain
        // a request in reality! Now, this could only happen if the head was
        // deleted and thus the head changed and thus we check for it.
        // Consider the same scenario, but now consider that the head is also
        // advanced so that new head == new tail. This check will not capture
        // this, but that is not a problem, because head==tail and so even
        // though at the time we determined wrongly that the queue is empty,
        // meanwhile it had become empty and thus check check should pass.
        if(myHead != head().load(std::memory_order_relaxed)) continue;

        // At this point we can be sure myTail is safe to use, because we
        // marked it as hazardous and we checked that no thread changed the
        // tail. The only way the tail could have been deleted is when the
        // head caught up with the tail and another thread FIRST advanced the
        // head (=tail) at that point. So hazard pointers combined with that
        // check guarantees myTail is safe to use.

#ifndef NDEBUG
        incrRefCount(myTail.ptr(), __LINE__);
#endif

        ctptr next = myTail.ptr()->_next.load(std::memory_order_relaxed);

        if (true) {
          // we can optimize out the check for (myTail == tail().loadOP()) here
          if (next.ptr() != nullptr) {
            // tail falling behind
            (void) cas(tail(), myTail, ctptr(next.ptr(), next.isRequest()));
          } else {    // try to link in the new node
            if (cas(myTail.ptr()->_next, next, ctptr(node, false))) {
              (void) cas(tail(), myTail, ctptr(node, false));
#ifdef MARE_DQ_DEBUG_ELEMENTS
              // std::cout << "[" << mare_thread_id() << "] PUSHED " <<
              //   element << " (size=" << ++_count << ")" << std::endl;
#endif
#ifdef MARE_DQ_DEBUG
              _count_push_fastPass++;
#endif
#ifndef NDEBUG
              decrRefCount(myTail.ptr(), __LINE__);
#endif
              // Unmark myTail as hazardous
              _HPManager.clearMySlot(0);

              // Done
              return;
            }
          }
        }
#ifndef NDEBUG
        decrRefCount(myTail.ptr(), __LINE__);
#endif
      } else {    // queue consists of requests

        // Mark myHead as hazardous
        _HPManager.setMySlot(0,myHead.ptr());

        if(myHead != head().load(std::memory_order_relaxed)) continue;

        // At this point we can be sure myHead is safe to use, because we
        // marked it as hazardous and we checked that no thread changed the
        // head. The only way the head could have been deleted is when
        // another thread FIRST advanced the head. So hazard pointers combined
        // that check guarantees myHead is safe to use.

#ifndef NDEBUG
        incrRefCount(myHead.ptr(), __LINE__);
#endif

        ctptr next = myHead.ptr()->_next.load(std::memory_order_relaxed);

        // Is this "myTail == tail().load(std::memory_order_relaxed)"
        // even needed? All it 'protects' against is a change to the
        // tail, all the other changes are captured by the head check.
        if (myTail == tail().load(std::memory_order_relaxed) &&
            next.ptr() != nullptr) {
          // tail has not changed
          cptr req = myHead.ptr()->_request.load(std::memory_order_relaxed);
          if (myHead == head().load(std::memory_order_relaxed)) {
            // head, next, and req are consistent

            // IMPORTANT NOTE: the following cas_strong() is ONLY safe if
            // myHead is NOT explicitly freed. Thus, it is safe when using
            // hazard pointers, since the queue then does not explicitly free
            // myHead.

            // Try to fulfill the request at the head
            // This is cas_strong because a few lines further we advance the
            // head. If this were to spuriously fail, we would try to advance
            // an UNFULFILLED head, causing loss of data.
            bool success = (req.ptr() == nullptr &&
                            cas_strong(myHead.ptr()->_request,
                                       req, cptr(node)));

#ifndef NDEBUG
            MARE_INTERNAL_ASSERT(node, "Node is null");
            verifyQNode(myHead.ptr(), __LINE__);
            if ((myHead.ptr()->
                 _request.load(std::memory_order_relaxed).ptr() ==
                 nullptr)) {
              std::stringstream s;
              _HPManager.print(s);
              MARE_FATAL("Fulfilling request failed! head:%p "
                         "success:%d, req:%p\n%s",
                         myHead.ptr(),success, req.ptr(), s.str().c_str());
            }
            MARE_INTERNAL_ASSERT(next.ptr(), "tried to set head() to null!");
#endif

            // Try to remove fulfilled request even if it's not mine
            // This has to be strong because a few lines further, if !success,
            // we retire myHead. If this were to spuriously fail, we would
            // retire a node that is still in the queue.
            (void)advanceHeadStrong(myHead, next);

            // If successfully attached a datanode to the request node,
            // notify the waiting pop()
            if (success) {
#ifdef MARE_DQ_DEBUG_ELEMENTS
              //std::cout << "[" << mare_thread_id() << "] PUSHED " <<
              //  element << " (size=" << ++_count << ")" << std::endl;
#endif
#ifdef MARE_DQ_DEBUG
              if(!next.isRequest()) {
                MARE_FATAL("linked data to node that is not a request! "
                           "DATA LOST!");
              }
#endif
              // notify the pop() waiting on this element
              if (do_notify)
                notify(myHead.ptr());
#ifdef MARE_DQ_DEBUG
              _count_push_slowPass++;
#endif
#ifndef NDEBUG
              decrRefCount(myHead.ptr(), __LINE__);
#endif
              // Unmark myHead as hazardous
              _HPManager.clearMySlot(0);

              // Done
              return;

              // If attaching a datanode failed, there can be two reasons:
              //   - Another push() beat this push() to it
              //   - The pop() bailed out prematurely, leaving an
              //     abandoned node
            } else {
              req = myHead.ptr()->_request.load(std::memory_order_relaxed);
              // If adding the data did not work, check if its an abandoned
              // node and if so, try to claim deletion. Then start over.
              // IMPORTANT NOTE: this is ONLY safe if the memory is NOT freed
              if(req.ptr() == MARE_CPTR_DELETEME_PTR &&
                 cas_strong(myHead.ptr()->_request, req,
                            cptr(MARE_CPTR_CLAIMDELETION_PTR))) {
#ifndef NDEBUG
                myHead.ptr()->_deletedMark = 1;
                myHead.ptr()->_deletedThread = thread_id();
#endif

#ifndef NDEBUG
                decrRefCount(myHead.ptr(), __LINE__);
#endif

                // Unmark myHead as hazardous
                _HPManager.clearMySlot(0);

                // We can safely retire this node, because it is out of the
                // queue and thus out of reach of newly created threads
#ifndef NDEBUG
                verifyQNode(myHead.ptr(), __LINE__);
#endif
                _HPManager.retireNode(myHead.ptr());

                // Not an abandoned node or another thread cleaned it up: give
                // up myHead and try again
              } else {
#ifndef NDEBUG
                decrRefCount(myHead.ptr(), __LINE__);
#endif
              }
            }

            // Head changed: give up myHead and try again
          } else {
#ifndef NDEBUG
            decrRefCount(myHead.ptr(), __LINE__);
#endif
          }

          // Tail changed: give up myHead and try again
          // XXX Optimization: don't give up on myHead, see popIf() for
          // more comments about the same optimization.
        } else {
#ifndef NDEBUG
          decrRefCount(myHead.ptr(), __LINE__);
#endif
        }
      }
    }
  }

  ///
  /// Dequeue an element. If not available, wait until one is available
  /// OR the specified predicate becomes false.
  /// @param pred Bailout if this predicate becomes false.
  /// @param element Will be the dequeued element if available.
  /// @return true iff an element was dequeued
  ///
  template <typename PREDICATE>
  bool popIf(PREDICATE pred, T& element, paired_mutex_cv* synch) {

    // Setup new qnode
    qnode* node = new qnode();
    MARE_INTERNAL_ASSERT(node, "node init failure");

    node->_next.storeNA(ctptr(nullptr, true));

#ifndef NDEBUG
    typename hp::Manager<qnode,2>::CheckCleanOnDestroy checkClean(_HPManager);
#endif

    while (1) {
      cptr myHead = head().load(std::memory_order_relaxed);

      // Mark myHead as hazardous
      _HPManager.setMySlot(0,myHead.ptr());

      if(myHead != head().load(std::memory_order_relaxed)) continue;

      // At this point we can be sure myHead is safe to use, because we
      // marked it as hazardous and we checked that no thread changed the
      // head. The only way the head could have been deleted is when
      // another thread FIRST advanced the head. So hazard pointers combined
      // that check guarantees myHead is safe to use.

#ifndef NDEBUG
      incrRefCount(myHead.ptr(), __LINE__);
#endif

      ctptr myTail = tail().load(std::memory_order_relaxed);

      // Check the state of the queue
      if ((myTail.ptr() == myHead.ptr()) || myTail.isRequest()) {

        // Mark myHead as hazardous
        _HPManager.setMySlot(1,myTail.ptr());

        if(myTail != tail().load(std::memory_order_relaxed)) {
#ifndef NDEBUG
          decrRefCount(myHead.ptr(), __LINE__);
#endif
          // XXX Possibility for optimization: save the current myHead to
          // safeHead, at the beginning of the loop check if myHead==safeHead,
          // if so:
          //   - skip marking it as hazardous
          //   - skip checking myHead==head()
          //   - skip refcount++
          // if not:
          //   - safeHead.ptr()->decrRefCount(__LINE__);
          //   - mark myHead as hazardous
          //   - check myHead==head()
          //   - incrRefCount(myHead.ptr(), __LINE__);
          // Furthermore: remove the decrRefCount above and init safeHead to
          // nullptr before the while.
          continue;
        }

        // At this point we can be sure myTail is safe to use, because we
        // marked it as hazardous and we checked that no thread changed the
        // head. The only way the head could have been deleted is when
        // another thread FIRST advanced the head. So hazard pointers combined
        // that check guarantees myHead is safe to use.

#ifndef NDEBUG
        incrRefCount(myTail.ptr(), __LINE__);
#endif

        // queue empty, tail falling behind, or queue contains data
        // (queue could also contain exactly one outstanding request
        // with tail pointer as yet unswung)
        ctptr next = myTail.ptr()->_next.load(std::memory_order_relaxed);
        if(true) {
          // we can optimize out the check for (myTail == tail().loadOP()) here
          if (next.ptr() != nullptr) {     // tail falling behind
            (void) cas(tail(), myTail, ctptr(next.ptr(), next.isRequest()));
          } else {    // try to link in a request for data
            if (cas(myTail.ptr()->_next, next, ctptr(node, true))) {

              myTail.ptr()->init_synch(synch);

              // linked in request; now try to swing tail pointer
              (void) cas(tail(), myTail, ctptr(node, true)); //{
              // help someone else if I need to
              if (myHead == head().load(std::memory_order_relaxed) &&
                  myHead.ptr()->
                  _request.load(std::memory_order_relaxed).ptr() != nullptr) {
                (void)advanceHead(myHead,
                                  myHead.ptr()->_next.load(
                                    std::memory_order_relaxed));
              }

              // initial linearization point
              cptr req(MARE_CPTR_DELETEME_PTR);

              // Specifies if we can safely read data from the node
              bool dataOK = true;

              // Lock & Wait
              {
                std::unique_lock<std::mutex> lock(myTail.ptr()->_synch->first);
                while(1) {
                  // Check if there's data in this dequeuer's node
                  bool empty =
                    myTail.ptr()->
                    _request.load(std::memory_order_relaxed).ptr() == nullptr;

                  // If there's no data and dequeuer still wants data
                  if(pred() && empty) {
#ifdef MARE_DQ_DEBUG_ELEMENTS
                    //std::cout << "[" << thread_id() << "] waiting (" <<
                    //  myTail.ptr() << ") " << (++_waiting) <<
                    //  " (size=" << _count << ")" << std::endl;
#endif
                    // Wait
#ifndef NDEBUG
                    if(myTail.ptr()->_waiter) {
                      MARE_FATAL("Already someone waiting on node!");
                    }
                    if(myTail.ptr()->_dataObtained) {
                      MARE_FATAL("Data already obtained! (line:%d)",__LINE__);
                    }
                    if(myTail.ptr()->_deleted) {
                      MARE_FATAL("Node deleted!");
                    }
                    myTail.ptr()->_waiter = thread_id();
#endif
                    myTail.ptr()->_synch->second.wait(lock);
#ifndef NDEBUG
                    MARE_INTERNAL_ASSERT(
                      myTail.ptr()->_waiter == thread_id(),
                      "For some reason this is not the node I "
                      "was waiting on!");
                    myTail.ptr()->_waiter = {0};
#endif

#ifdef MARE_DQ_DEBUG_ELEMENTS
                    //std::cout << "[" << thread_id() << "] woke up (" <<
                    //  myTail.ptr() << ") " << (--_waiting) <<
                    //  " (size=" << _count << ")" << std::endl;
#endif
                    // If dequeuer doesn't want to wait anymore
                  } else if(empty) {
                    // This means pred() is false. Try to bail out.
                    // Start by marking this node as an abandoned node.
                    // Set dataOK to false to specify to NOT try to read data.
                    req = myTail.ptr()->_request.load(
                            std::memory_order_relaxed);
                    if(req.ptr()==nullptr) {
                      dataOK = !cas_strong(myTail.ptr()->_request, req,
                                           cptr(MARE_CPTR_DELETEME_PTR));
                    }
                    break;

                    // If there's data and dequeuer wants it
                  } else {
                    break;
                  }
                }
              }

              // Get the pointer to a data qnode
              // It can be one of the following:
              //   - MARE_CPTR_DELETEME      successfully marked abandoned
              //   - MARE_CPTR_CLAIMDELETION marked and already deleted by
              //     enqueuer
              //   - valid pointer      enqueuer managed to link datanode
              // If dataOK is false, set to nullptr
              qnode* dataNode = dataOK ?
                (myTail.ptr()->
                 _request.load(std::memory_order_relaxed).ptr()) :
                nullptr;

#ifdef MARE_DQ_DEBUG
              _count_pop_slowPass++;
#endif

              // If there's data to be had, collect it
              // XXX do dataOK here instead of above
              if(dataNode>MARE_CPTR_CLAIMDELETION_PTR) {
#ifndef NDEBUG
                bool b = false;
                if(!myTail.ptr()->
                   _dataObtained.compare_exchange_strong(b,true)) {
                  MARE_FATAL("Data already obtained! (line:%d)", __LINE__);
                }
#endif
                // Obtain the element from the data node
                element = dataNode->_element;

                // help snip my node
#ifndef NDEBUG
                cptr origHead = myHead;
#endif

                // Make sure we advance the head if its my node, otherwise
                // the node cannot be retired.
                myHead = head().load(std::memory_order_relaxed);
                if (myHead.ptr() == myTail.ptr()) {
                  // This has to be strong because a few lines
                  // further, we retire myHead. If this were to
                  // spuriously fail, we would retire a node that is
                  // still in the queue.
                  (void)advanceHeadStrong(myHead, cptr(node));
                }

                // The data node we can safely explicitly delete, because
                //   - it cannot be referenced by any other dequeuer; and
                //   - the enqueuer that linked it in does not use it after
                //     linking it in; and
                //   - it cannot be referenced by any other enqueuers.
                delete dataNode;

                // This delete is unsafe in the original algorithm (without
                // hazard pointers) and also when using alignedatomic-slow.hh
                //delete myTail.ptr();
#ifndef NDEBUG
                myTail.ptr()->_deletedMark = 2;
                myTail.ptr()->_deletedThread = thread_id();
                decrRefCount(myTail.ptr(), __LINE__);
                decrRefCount(origHead.ptr(), __LINE__);
#endif
                // Unmark myHead and myTail as hazardous
                _HPManager.clearMySlot(0);
                _HPManager.clearMySlot(1);

                // We can safely retire this node, because it is out of the
                // queue and thus out of reach of newly created threads
#ifndef NDEBUG
                verifyQNode(myTail.ptr(), __LINE__);
#endif
                _HPManager.retireNode(myTail.ptr());

#ifdef MARE_DQ_DEBUG_ELEMENTS
                //std::cout << "[" << thread_id() <<
                //  "] POPPED after wait: " << element <<
                //  " (size=" << --_count << ")" << std::endl;
#endif
                // Signal there was data dequeued
                return true;
              } else {

#ifdef MARE_DQ_DEBUG_ELEMENTS
                //std::cout << "[" << thread_id() <<
                //  "] POPPED nothing (size=" << --_count << ")" <<
                // std::endl;
#endif
#ifndef NDEBUG
                decrRefCount(myTail.ptr(), __LINE__);
                decrRefCount(myHead.ptr(), __LINE__);
#endif

                // Unmark myHead and myTail as hazardous
                _HPManager.clearMySlot(0);
                _HPManager.clearMySlot(1);

                // Signal there was NO data dequeued
                return false;
              }
            }
          }
        }
#ifndef NDEBUG
        decrRefCount(myTail.ptr(), __LINE__);
#endif
      } else {    // queue consists of real data

        // Load head.next
        ctptr next = myHead.ptr()->_next.load(std::memory_order_relaxed);

        // Mark next node as hazardous.
        // Note that if it was deleted between checking the head for
        // consistency and marking next as used, this means the head has
        // changed and the next check of head will catch this and fail.
        _HPManager.setMySlot(1,next.ptr());

        // Check for consistency
        if (myHead == head().load(std::memory_order_relaxed) && myTail ==
            tail().load(std::memory_order_relaxed) && next.ptr() != nullptr) {
          // head and next are consistent; read result *before* swinging head

          // At this point we can be sure next is safe to use, because
          // we marked it as hazardous and we checked that no thread
          // changed the head. The only way next could have been
          // deleted is when head caught up with next and another
          // thread FIRST advanced the head.  So hazard pointers
          // combined that check guarantees myHead is safe to to use.

          MARE_INTERNAL_ASSERT(
            next.ptr() != nullptr,
            "next should not be null");

#ifndef NDEBUG
          incrRefCount(next.ptr(), __LINE__);
#endif

          // Try to claim the head by advancing the head
          if (advanceHead(myHead, next)) {

#ifndef NDEBUG
            MARE_INTERNAL_ASSERT(
              myHead.ptr()->_request.load(std::memory_order_relaxed)._ptr <=
              MARE_CPTR_CLAIMDELETION_PTR,
              "Dequeuer snipped a data node WITH a data node!");
            MARE_INTERNAL_ASSERT(
              !next.isRequest(),
              "Dequeur snipped a request node");
            verifyQNode(next.ptr(), __LINE__);
#endif

            // This delete is unsafe in the original algorithm (without
            // hazard pointers) and also when using alignedatomic-slow.hh
            //delete myHead.ptr();
#ifndef NDEBUG
            myHead.ptr()->_deletedMark = 3;
            myHead.ptr()->_deletedThread = thread_id();
            decrRefCount(myHead.ptr(), __LINE__);
#endif

            // Unmark myHead as hazardous
            _HPManager.clearMySlot(0);

            // We can safely retire this node, because it is out of the
            // queue and thus out of reach of newly created threads
#ifndef NDEBUG
            verifyQNode(myHead.ptr(), __LINE__);
#endif
            _HPManager.retireNode(myHead.ptr());

            // Delete uselessly allocated node
            // FIXME: optimization: never alloc node
            delete node;

#ifdef MARE_DQ_DEBUG_ELEMENTS
            //std::cout << "[" << thread_id() << "] POPPED: " << element
            // << " (size=" << --_count << ")" << std::endl;
#endif
#ifdef MARE_DQ_DEBUG
            _count_pop_fastPass++;
#endif
#ifndef NDEBUG
            bool b = false;
            if(!next.ptr()->
               _dataObtainedFast.compare_exchange_strong(b,true)) {
              MARE_FATAL("Data already obtained! (line:%d)",__LINE__);
            }
#endif
            // Return data
            element = next.ptr()->_element;

            // Unmark next node as used
#ifndef NDEBUG
            decrRefCount(next.ptr(), __LINE__);
#endif
            // Unmark next as hazardous
            _HPManager.clearMySlot(1);

            // Done
            return true;
          }
#ifndef NDEBUG
          decrRefCount(next.ptr(), __LINE__);
#endif
        }
        // Unmark next node as hazardous
        _HPManager.clearMySlot(1);
      }
#ifndef NDEBUG
      decrRefCount(myHead.ptr(), __LINE__);
#endif
    }
  }

  ///
  /// Pop an element from the queue. If there is no element, block until
  /// there is one.
  /// @return The dequeued element.
  ///
  T pop(paired_mutex_cv* synch)
  {
    T element;
    if (popIf([] { return true; }, element, synch))
      return element;
    return nullptr;
  }

  ///
  /// Pop an element from the queue. If there is no element, block until
  /// there is one. The waiting will be done on the specified condition
  /// variable.
  /// @param element The dequeued element will be written to this reference.
  /// @param condvar The condition variable on which will be waited if needed.
  /// @return true iff an element was dequeued.
  ///
  bool pop(T& element, paired_mutex_cv* synch)
  {
    if (popIf([] { return true; }, element, synch))
      return true;
    return false;
  }

//  ///
//  /// Unsafely prints the current state of the queue.
//  ///
//  void print(int upto=(1<<30)) {
//    qnode* node = head().load().ptr();
//    int number = 0;
//    while(node) {
//      std::cout.width(4);
//      std::cout << std::right << std::dec << number << ": " << node <<
//        " -> ";
//      node->print(std::cout);
//      std::cout << std::endl;
//      ++number;
//      if(number==upto) break;
//      node = node->_next.load().ptr();
//    }
//  }
//
//  /// Find
//  ///
//   void findNode(qnode* matchNode) {
//    qnode* node = head().load().ptr();
//    int number = 0;
//    bool found = false;
//    while(node) {
//      if(matchNode == node) {
//        found = true;
//        std::cout.width(4);
//        std::cout << std::right << std::dec << number << ": ";
//        node->print(std::cout);
//        std::cout << std::endl;
//      }
//      ++number;
//      node = node->_next.load().ptr();
//    }
//    if(!found) std::cout << "Not found node: " << matchNode << std::endl;
//   }

  void notify(struct qnode* node) {
    if(node->_synch != nullptr){
      std::unique_lock<std::mutex> lock(node->_synch->first);
      node->_synch->second.notify_one();
    }
  }

  ///
  /// Initialize the calling thread for making use of this queue.
  /// Call threadFinish() when the thread is done, otherwise there may be
  /// memory leaks.
  ///
  void threadInit() {
    _HPManager.threadInit();
  }

  ///
  /// Initialize the calling thread for making use of this queue if needed.
  /// Call threadFinish() when the thread is done, otherwise there may be
  /// memory leaks.
  ///
  void threadInitAuto() {
    _HPManager.threadInitAuto();
  }

  //
  // Signals this thread is done using the queue.
  //
  void threadFinish() {
    _HPManager.threadFinish();
  }

#ifndef NDEBUG
  void verifyQNode(struct qnode* node, int line) {
    if(node->_deleted) {
      std::stringstream s;
      s << "Node should not be deleted at line " << line << "! node: " <<
        node << " del: " << node->_deleted << " mark: " <<
        node->_deletedMark << " retiredBy: " << node->_deletedThread;
      _HPManager.print(s);
      MARE_FATAL("%s",s.str().c_str());
    }
  }

  void incrRefCount(struct qnode* node, int line) {
    MARE_INTERNAL_ASSERT(
      node->_refCount>=0,
      "Reference count of node %p is negative",
      this);
    node->_refCount++;
    verifyQNode(node, line);
  }

  void decrRefCount(struct qnode* node, int line) {
    verifyQNode(node, line);
    node->_refCount--;
    MARE_INTERNAL_ASSERT(
      node->_refCount>=0,
      "Reference count of node %p is negative",
      this);
  }
#endif

public:

  struct cptr {
    struct qnode* _ptr;

    cptr() MARE_NOEXCEPT:
    _ptr(nullptr)
    {
    }

    cptr(struct qnode* const& p):
      _ptr(p)
    {
    }

    cptr(const cptr& other):
      _ptr(other._ptr)
    {
    }

    inline qnode* ptr() const {
      return _ptr;
    }

    inline qnode* ptr(qnode* p) {
      _ptr = p;
      return _ptr;
    }

    bool operator==(const cptr& other) const {
      return   _ptr == other._ptr;
    }

    bool operator!=(const cptr& other) const {
      return !(*this==other);
    }

    const cptr& operator=(const cptr& other) {
      if(this!=&other) {
        this->_ptr = other._ptr;
      }
      return *this;
    }
  }
    ;

  struct ctptr {
  private:
    struct qnode* _ptr;
  public:

    inline qnode* ptr() const {
      return reinterpret_cast<qnode*>
        (reinterpret_cast<intptr_t>(_ptr) & (~1UL));
    }

    inline qnode* ptr(qnode* p) {
      _ptr = reinterpret_cast<qnode*>(reinterpret_cast<intptr_t>(p) |
                                      (reinterpret_cast<intptr_t>(_ptr) &
                                       (~1UL)));
      return _ptr;
    }

    inline bool isRequest() const {
      return (reinterpret_cast<intptr_t>(_ptr)&1);
    }

    inline bool isRequest(bool isReq) const {
      _ptr = reinterpret_cast<qnode*>((reinterpret_cast<intptr_t>(_ptr)&(~1)) |
                                      (isReq != false));
      MARE_INTERNAL_ASSERT(isReq == this->isRequest(),
                           "bit stuffing error");
      return isReq;
    }

    ctptr() MARE_NOEXCEPT:
    _ptr(nullptr)
    {
    }

    ctptr(qnode* ptrb, bool isReq):
      _ptr(reinterpret_cast<qnode*>(reinterpret_cast<intptr_t>(ptrb) |
                                    (isReq != false)))
    {
      MARE_INTERNAL_ASSERT(isReq == this->isRequest(),
                           "bit stuffing error");
    }

    ctptr(const ctptr& other):
      _ptr(other._ptr)
    {
    }

    bool operator==(const ctptr& other) const {
      return   _ptr == other._ptr;
    }

    bool operator!=(const ctptr& other) const {
      return !(*this==other);
    }

    const ctptr& operator=(const ctptr& other) {
      if(this!=&other) {
        this->_ptr = other._ptr;
      }
      return *this;
    }
  }
    ;

  struct qnode {
    alignedatomic<cptr> _request;
    alignedatomic<ctptr> _next;
#ifndef NDEBUG
    uintptr_t _waiter;
    std::atomic<bool> _dataObtained;
    std::atomic<bool> _dataObtainedFast;
    bool _deleted;
    int _deletedMark;
    uintptr_t _deletedThread;
    std::atomic<int> _refCount;
#endif
    paired_mutex_cv* _synch;
    T _element;
#ifdef MARE_DQ_DEBUG_MEMORY
    static std::atomic<size_t> _allocs;
    static std::atomic<size_t> _deletes;
#endif

    // Override operator new and delete if macro defined in memdebug.hh
    MARE_OPERATOR_NEW_OVERRIDE

    MARE_DELETE_METHOD(qnode& operator=(qnode const&));

    MARE_DELETE_METHOD(qnode(qnode const&));
    qnode():
      _request(),
      _next(),
#ifndef NDEBUG
      _waiter(0),
      _dataObtained(false),
      _dataObtainedFast(false),
      _deleted(false),
      _deletedMark(0),
      _deletedThread(0),
      _refCount(0),
#endif
      _synch(nullptr),
      _element() {
    }
    qnode(const T& element):
      _request(),
      _next(),
#ifndef NDEBUG
      _waiter(0),
      _dataObtained(false),
      _dataObtainedFast(false),
      _deleted(false),
      _deletedMark(0),
      _deletedThread(0),
      _refCount(0),
#endif
      _synch(nullptr),
      _element(element) {
    }

#ifndef NDEBUG
    ~qnode() {
      int rc = _refCount.load(std::memory_order_relaxed);
      if(rc>0) {
        MARE_FATAL("QNode still in use, but deleted! "
                   "node: %p rc: %d mark: %i", this, rc, _deletedMark);
      } else if(rc<0) {
        MARE_FATAL("QNode has negative refCount! "
                   "node: %p rc: %d mark: %i", this, rc, _deletedMark);
      }
      if(_deleted) {
        MARE_FATAL("QNode already deleted!? Marked: %d", _deletedMark);
      }
      _deleted = true;
    }
#endif

    void init_synch(paired_mutex_cv* synch){
      if(_synch == nullptr){
        _synch = synch;
      }
    }

#ifdef MARE_DQ_DEBUG_MEMORY
    void* operator new(std::size_t size) {
      void* mem = malloc(size);
      if(mem) {
        _allocs++;
        return mem;
      } else {
        throw new std::bad_alloc();
      }
    }

    void* operator new(std::size_t size,
                       const std::nothrow_t& nothrow_constant) throw() {
      _allocs++;
      return malloc(size);
    }

    void* operator new(std::size_t size, void* ptr) throw() {
      return ptr;
    }

    void operator delete(void* ptr) throw () {
      MARE_INTERNAL_ASSERT(ptr, "ptr is null");
      _deletes++;
      free(ptr);
    }
    void operator delete(void* ptr,
                         const std::nothrow_t& nothrow_constant) throw() {
      MARE_INTERNAL_ASSERT(ptr, "ptr is null");
      _deletes++;
      free(ptr);
    }
    void operator delete (void* ptr, void* voidptr2) throw() {
    }
#endif

//    void print() {
//      print(std::cout);
//    }
//    void print(std::ostream& out) {
//      if(this->_next.load().isRequest()) {
//        out << "[REQ]  ";
//      } else {
//        out << "[DATA] ";
//      }
//      out.width(10);
//      out << this->_element;
//      if(this->_request.load().ptr()>MARE_CPTR_CLAIMDELETION_PTR) {
//        out << " (req: " << this->_request.load().ptr()->_element << ")";
//      } else if(this->_request.load().ptr()==MARE_CPTR_CLAIMDELETION_PTR) {
//        out << " (req: claim deletion)";
//      } else if(this->_request.load().ptr()==MARE_CPTR_DELETEME_PTR) {
//        out << " (req: delete me)";
//      } else {
//        out << " (req: no data)";
//      }
//#ifndef NDEBUG
//      if(this->_waiter != 0) {
//        out << " (waiter: " << this->_waiter << ")";
//      } else {
//        out << " (waiter: none)";
//      }
//#endif
//      if(this->_next.load().ptr()) {
//        out << " (next: " << this->_next.load().ptr() << ")";
//      } else {
//        out << " (next: none)";
//      }
//    }
  };

private:

  inline bool cas_strong(alignedatomic<ctptr>& ptr, ctptr expected,
                         const ctptr& newPtr) {
    return ptr.compare_exchange_strong(expected,newPtr);
  }
  inline bool cas_strong(alignedatomic<cptr>& ptr, cptr expected,
                         const cptr& newPtr) {
    return ptr.compare_exchange_strong(expected,newPtr);
  }

  inline bool cas(alignedatomic<ctptr>& ptr, ctptr expected,
                  const ctptr& newPtr) {
    return ptr.compare_exchange_weak(expected,newPtr);
//    std::unique_lock<std::mutex> lock(_mutex);
//    if(ptr==expected) {
//      ptr = newPtr;
//      return true;
//    } else {
//      //expected = ptr;
//      return false;
//    }
  }
  inline bool cas(alignedatomic<cptr>& ptr, cptr expected,
                  const cptr& newPtr) {
    return ptr.compare_exchange_weak(expected,newPtr);
//    std::unique_lock<std::mutex> lock(_mutex);
//    if(ptr==expected) {
//      ptr = newPtr;
//      return true;
//    } else {
//      //expected = ptr;
//      return false;
//    }
  }

  inline alignedatomic<cptr>& head() {
    return _head;
  }

  inline alignedatomic<ctptr>& tail() {
    return _tail;
  }

  ///
  /// Tries to advance the head using CAS.
  /// If it succeeds, it will return true. Otherwise, it will return false.
  /// It is allowed to spuriously fail; if you need it to not spuriously fail,
  /// use advanceHeadStrong().
  /// @param expectedHead The head that is expected to be the current head.
  /// @param newHead The head that is to become the new head.
  /// @return true iff the the head was advanced by this call.
  /// @ensure result -> head() == newHead
  ///
  bool advanceHead(const cptr& expectedHead, const cptr& newHead) {
    return cas(head(),expectedHead,newHead);
  }
  bool advanceHead(const cptr& expectedHead, const ctptr& newHead) {
    return advanceHead(expectedHead,cptr(newHead.ptr()));
  }

  bool advanceHeadStrong(const cptr& expectedHead, const cptr& newHead) {
    return cas_strong(head(),expectedHead,newHead);
  }

  bool advanceHeadStrong(const cptr& expectedHead, const ctptr& newHead) {
    return cas_strong(head(),expectedHead,cptr(newHead.ptr()));
  }

// Fields
private:
#ifdef MARE_DQ_DEBUG
public:
  std::atomic<size_t> _count;
  std::atomic<size_t> _waiting;
  std::atomic<size_t> _count_push_fastPass;
  std::atomic<size_t> _count_push_slowPass;
  std::atomic<size_t> _count_pop_fastPass;
  std::atomic<size_t> _count_pop_slowPass;
#endif
  alignedatomic<cptr> _head;
  alignedatomic<ctptr> _tail;
#if defined(MARE_DQ_OPTION_SINGLECONDVAR) || \
  defined(MARE_DQ_OPTION_EXTERNCONDVAR)
  std::condition_variable _condvar;
#endif
  std::mutex _mutex;
  hp::Manager<qnode,2> _HPManager;

};

#ifdef MARE_DQ_DEBUG_MEMORY
template<typename T>
typename std::atomic<size_t> DualTaskQueue<T>::qnode::_allocs;
template<typename T>
typename std::atomic<size_t> DualTaskQueue<T>::qnode::_deletes;
#endif

} //namespace internal

} //namespace mare
