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
#include <unordered_set>
#include <iostream>
#include <sstream>
#include <mare/internal/debug.hh>
#include <mare/internal/tlsptr.hh>

namespace mare {

namespace internal {

namespace hp {

///
/// Linked list node, to store retired nodes in.
///
template<typename T>
struct ListNode {
public:
  ListNode(ListNode* n, T e):
    _next(n),
    _element(e) {
  }
  void next(ListNode* const& n) {
    _next = n;
  }
  ListNode* const& next() const {
    return _next;
  }
  const T& element() const {
    return _element;
  }

  // Override operator new and delete if macro defined in memdebug.hh
  MARE_OPERATOR_NEW_OVERRIDE

  private:
  ListNode* _next;
  T _element;
};

///
/// A record of all the hazard pointers of one thread.
/// Each thread has its own hp::Record. To claim ownership of a record,
/// call tryClaim(). If it returns true, exclusive access is guaranteed
/// to the fields _active, _rList and _rCount. Otherwise not guarantees are
/// made. To give up a claim, call release().
///
template<typename TN, int K>
struct Record {
public:

  // Override operator new and delete if macro defined in memdebug.hh
  MARE_OPERATOR_NEW_OVERRIDE

  ///
  /// Constructs a new inactive record. Claim before using it.
  ///
  Record():
#ifndef NDEBUG
    _owner(0),
#endif
    _next(nullptr),
    _active(false),
    _rList(nullptr),
    _rCount(0),
    _nrThreads(0) {
    for(int idx=0; idx != K; idx++)
      _HP[idx] = nullptr;
#ifndef NDEBUG
    for(int idx=K; idx--;) {
      if(_HP[idx]) {
        MARE_FATAL("Record not clean");
      }
    }
#endif
  }

  ///
  /// Add node to the list of retired nodes.
  ///
  void push(TN* element) {
    ListNode<TN*>* newNode = new ListNode<TN*>(_rList,element);
    _rList = newNode;
    ++_rCount;
  }

  ///
  /// Pop the top-most (LIFO) retired node from the list and return it.
  ///
  void pop(TN*& element) {
    ListNode<TN*>* oldHead = _rList;
    _rList = _rList->next();
    element = oldHead->element();
    delete oldHead;
    --_rCount;
  }

  ///
  /// Scan all records for references and delete retired nodes in the list of
  /// this record that are not referenced anymore.
  /// NOTE: the caller has to have ownership (use tryClaim()).
  ///
  void scan(Record* headHPRecord) {

    //
    // Stage 1: go through all records to find out what pointers
    //          all threads are using. Store result in pList.
    //

    MARE_INTERNAL_ASSERT(headHPRecord->nrThreads()>0,
                         "nrThreads should be > 0");
#ifdef _MSC_VER
    // VS2012 cannot create variable length arrays, so use an
    // std::vector instead
    std::vector<TN*> pList_temp (headHPRecord->nrThreads()*K);
    TN** pList = pList_temp.data();
#else
    // volatile?
    TN* pList[headHPRecord->nrThreads()*K];
#endif

    TN** pIdx = pList;
    for(Record* hprec = headHPRecord; hprec != nullptr; hprec = hprec->_next) {
      for(int i=0; i<K; ++i) {
        TN* hptr = hprec->_HP[i];
        if(hptr) {
          *pIdx++ = hptr;
        }
      }
    }

    //
    // Stage 2: go through my list of nodes I want to retire and delete
    //          the nodes that are not used by any thread anymore.
    // We have unhindered access to rList and rCount.
    //

    ListNode<TN*>* last = nullptr;
    ListNode<TN*>* node = _rList;
    while(node) {

      bool found = false;
      for(TN** p = pList; p<pIdx; p++) {
        if(*p == node->element()) {
          found = true;
          break;
        }
      }

      // Check if the current node is still referenced somewhere
      // If not, remove it from the list and delete it
      if(!found) {

        if(last) {
          last->next(node->next());
          delete node->element();
          delete node;
          node = last->next();
        } else {
          _rList = node->next();
          delete node->element();
          delete node;
          node = _rList;
        }
        --_rCount;

        // If it's still referenced somewhere, leave it in the list
      } else {
        last = node;
        node = node->next();
      }
    }
  }

  ///
  /// Go through all the records to find inactive records. Add all nodes
  /// of inactive records to the list of retired nodes of this record.
  /// NOTE: the caller has to have ownership (use tryClaim()).
  /// @param headHPRecord The head of the hp::Record list in the hp::Manager.
  /// @param nodeThreshold The threshold of retired nodes after which a scan
  ///        called for the record over the threshold.
  ///
  void helpScan(std::atomic<Record*>& headHPRecord, const int& nodeThreshold) {
    for(Record* hprec = headHPRecord; hprec; hprec = hprec->_next) {
      if(!hprec->tryClaim()) continue;
      while(hprec->rCount()>0) {
        TN* node;
        hprec->pop(node);
        push(node);

        // Get the current head of the record list. This guarantees the scan
        // will pick up records that have been added since this helpScan()
        // was called.
        Record* head = headHPRecord;

        // At this point it is impossible for a newly created or claimed
        // Record to mark this node as hazardous, if the algorithm adheres to
        // the following demand:
        //   A newly initialized thread cannot know about retired nodes,
        //   meaning they should not be able to get a reference to one.
        // Only then can the node we just claimed from the record of a
        // finished thread be deleted.
        if(_rCount >= nodeThreshold) {
          scan(head);
        }
      }
      hprec->release();
    }
  }

  ///
  /// Retires this record for future reuse. All HP records are reset and
  /// ownership is relinquished.
  /// NOTE: the caller has to have ownership (use tryClaim()).
  ///
  void retire() {
    for(int k=K; k--;) {
      _HP[k] = nullptr;
    }
    release();
  }

  ///
  /// Returns whether his record is active or not.
  /// @return true iff this record is active.
  ///
  bool isActive() {
    return _active;
  }

  ///
  /// Tries to claim ownership of this record without blocking.
  /// Returns true iff claiming succeeded.
  /// @return true iff claiming succeeded.
  ///
  bool tryClaim() {
    if(_active) return false;
    bool b = false;
    b = _active.compare_exchange_strong(b, true);
#ifndef NDEBUG
    if(b) {
      // Prevent floating of the assignment before the check above.
      // Also make sure we observe _owner assignment in release()
      // before reading and assigning it here.
      __sync_synchronize();
      if (_owner) {
        MARE_FATAL("Thread got claim to already claimed record: %"
                   MARE_FMT_TID,
               _owner);
      }
      _owner = thread_id();
    }
#endif
    return b;
  }

  ///
  /// Relinquish ownership of this record.
  ///
  void release() {
#ifndef NDEBUG
    if(_owner != thread_id()) {
      MARE_FATAL("FATAL: Thread released record that is not his: "
             "%" MARE_FMT_TID "/%" MARE_FMT_TID,
             _owner, thread_id());
    }
    _owner = 0;
    // Prevent floating of the assignment after _active; otherwise,
    // this can cause the cmpxchg in tryClaim() to succeed before the
    // above assignment to _owner, which then subsequently overwrites
    // its assignment in tryClaim().
    //
    // We might miss for a while that the record is inactive, but that
    // just causes it to be freed a little later, hence no problem.
    __sync_synchronize();
#endif
    _active = false;
  }

  const int& rCount() const {
    return _rCount;
  }

  Record* const& next() const {
    return _next;
  }

  void next(Record* const& n) {
    _next = n;
  }

  TN* const & operator[](size_t slot) const {
    return _HP[slot];
  }

  void setSlot(size_t slot, TN* const& hp) {
    _HP[slot] = hp;
  }

  void clearSlot(size_t slot) {
    _HP[slot] = nullptr;
  }

  void print(std::ostream& out) {
    {
      out << "[";
      if(_active) {
        out << "ACT";
      } else {
        out << "OFF";
      }
      out << "|" << this << "]";
    }
#ifndef NDEBUG
    {
      out << "[";
      if(_owner) {
        out.setf(out.hex);
        out.width(16);
        out << _owner;
      } else {
        out << "      free      ";
      }
      out << "]";
    }
#endif
    {
      out << "[";
      bool first = true;
      for(int i = 0; i<K; ++i) {
        if(first) {
          first = false;
        } else {
          out <<"|";
        }
        out.width(16);
        out << _HP[i];
      }
      out << "]";
    }
    {
      out << " ";
      bool first = true;
      for(ListNode<TN*>* rList = _rList; rList; rList = rList->next()) {
        if(first) {
          first = false;
        } else {
          out <<", ";
        }
        out.width(16);
        out << rList;
      }
    }
    out << std::endl;
  }

  void nrThreads(size_t nThreads) {
    _nrThreads = nThreads;
  }
  const size_t& nrThreads() {
    return _nrThreads;
  }

public:
#ifndef NDEBUG
  uintptr_t _owner;
#endif
  TN* _HP[K];
  Record* _next;
  std::atomic<bool> _active;
  ListNode<TN*>* _rList;
  int _rCount;
  size_t _nrThreads;
};

///
/// The hp::Manager manages hazard pointers of one data structure. Before using
/// it, call threadInit() once in EACH thread using the datastructure. This
/// will setup the TLS needed. If you cannot guarantee it is only called once
/// per thread, for example you have no control over the start of the thread,
/// use threadInitAuto() at a point where you know that thread will be before
/// using this hp::Manager. This checks for valid TLS setup on each invocation.
/// When a thread is done, call threadFinish() in that thread, to signal that
/// it is done.
///
template<typename TN, int K>
class Manager {
public:
  typedef Record<TN,K>* Record_p;
  static int const MINIMAL_RLIST_SIZE;

  class CheckCleanOnDestroy {
  public:
    CheckCleanOnDestroy(Manager<TN,K>& HPManager): _HPManager(HPManager) {

    }
    ~CheckCleanOnDestroy() {
      for(int idx=K; idx--;) {
        if(_HPManager[idx]) {
          std::stringstream s;
          _HPManager.print(s);
          MARE_FATAL("Record not clean - %s",s.str().c_str());
        }
      }
    }
  private:
    Manager<TN,K>& _HPManager;
  };

public:

  Manager(const int& nodeThreshold = MINIMAL_RLIST_SIZE):
    _headHPRecord(nullptr),
    _recordCount(0),
    _nodeThreshold(nodeThreshold),
    _myhprec(nullptr) {
  }

  ///
  /// Destructs this manager and deletes all associated nodes.
  /// NOTE: ALL records should be inactive, so ALL threads have to have called
  /// their clean up method threadFinish() before this is called.
  ///
  ~Manager() {
    return; // @todo Manager still active in non-MARE threads
    // NOT REACHED
    for(Record_p hprec = _headHPRecord; hprec!=nullptr;
        hprec = hprec->next()) {
      if(hprec->isActive()) {
        MARE_FATAL("Manager deleted, but a record is still active");
        //print();
      }
      hprec->scan(_headHPRecord);
    }
  }

  ///
  /// Initialize the calling thread for making use of this manager. It will
  /// setup a new hp::Record for the thread.
  /// Call threadFinish() when the thread is done, otherwise there may be
  /// memory leaks.
  ///
  void threadInit() {
    MARE_INTERNAL_ASSERT(
      !_myhprec,
      "Please call threadInit() only on an uninitialized thread");
    allocRecord(_myhprec);
  }

  ///
  /// Initialize the calling thread for making use of this manager if needed.
  /// It will setup a new hp::Record for the thread if needed.
  /// Call threadFinish() when the thread is done, otherwise there may be
  /// memory leaks.
  ///
  void threadInitAuto() {
    if(!_myhprec) {
      allocRecord(_myhprec);
    }
  }

  ///
  /// Signals this thread is done using the datastructure the hp::Manager is
  /// taking care of. It will make sure the thread is not claiming any nodes
  /// that can be freed otherwise.
  /// As opposed to threadFinish(), you may call this multiple times, because
  /// it does an additional check.
  ///
  void threadFinishAuto() {
    if(_myhprec) {
      threadFinish();
    }
  }

  ///
  /// Signals this thread is done using the datastructure the hp::Manager is
  /// taking care of. It will make sure the thread is not claiming any nodes
  /// that can be freed otherwise.
  /// Call only once per thread.
  ///
  void threadFinish() {
    MARE_INTERNAL_ASSERT(
      _myhprec,
      "Please call threadInit() on each thread using hp::Manager");
    _myhprec->retire();
    _myhprec = nullptr;
  }

  ///
  /// Signal that the specified node is to be deleted as soon as no thread
  /// is using it anymore.
  /// Note that threadInit() has to be called for the calling thread.
  /// IMPORTANT: You have to make sure the specified node cannot be known to
  /// threads initialized after this call.
  /// @param node Node to be deleted.
  ///
  void retireNode(TN* node) {
    retireNode(_myhprec,node);
  }

  ///
  /// Access the specified slot of hazard pointers, read-only.
  /// @param slot The hazard pointer slot of the current thread. 0<=slot<K
  /// @return Const reference to the node pointer in the specified slot.
  ///
  TN* const& operator[](size_t slot) {
    MARE_INTERNAL_ASSERT(
      _myhprec,
      "Please call threadInit() on each thread using hp::Manager");
    return (*_myhprec)[slot];
  }

  ///
  /// Mark the specified node as being hazardous by assigning it to the HP
  /// list of the hp::Record of the calling thread in the specified slot.
  /// To unmark it as hazardous, use clearMySlot().
  /// @param slot The hazard pointer slot of the current thread. 0<=slot<K
  /// @param hp Pointer to the node to be marked as hazardous.
  ///
  void setMySlot(size_t slot, TN* const& hp) {
    _myhprec->setSlot(slot,hp);
    __sync_synchronize();
  }

  ///
  /// Clear the specified slot in the list of the hp:Record of the calling
  /// thread. The pointer that was in that slot is no longer marked as
  /// hazardous.
  /// @param slot The hazard pointer slot of the current thread. 0<=slot<K
  ///
  void clearMySlot(size_t slot) {
    _myhprec->clearSlot(slot);
  }

  ///
  /// Unsafely prints the state of all the records.
  ///
  void print() {
    print(std::cerr);
  }
  void print(std::ostream& out) {
    for(Record_p hprec = _headHPRecord; hprec; hprec = hprec->next()) {
      hprec->print(out);
    }
  }

  ///
  /// Safely prints the state of all the records.
  ///
  void printSafe(std::ostream& out) {
    for(Record_p hprec = _headHPRecord; hprec; hprec = hprec->_next) {
      if(hprec->tryClaim()) {
        hprec->print(out);
      } else {
        out << "Could not claim: " << hprec << std::endl;
      }
    }
  }

  bool isUsed(TN* node) {
    Record_p headHPRecord = _headHPRecord;
    MARE_INTERNAL_ASSERT(
      headHPRecord->nrThreads()>0,
      "nrThreads should be > 0");
#ifdef _MSC_VER
    // VS2012 cannot create variable length arrays, so use an
    // std::vector instead
    std::vector<TN*> pList_temp (headHPRecord->nrThreads()*K);
    TN** pList = pList_temp.data();
#else
    // volatile?
    TN* pList[headHPRecord->nrThreads()*K];
#endif

    TN** pIdx = pList;
    for(Record_p hprec = headHPRecord; hprec != nullptr;
        hprec = hprec->_next) {
      for(int i=0; i<K; ++i) {
        TN* hptr = hprec->_HP[i];
        if(hptr == node) {
          return true;
        }
      }
    }

    return false;
  }

  bool isUsedExceptByMe(TN* node) {
    Record_p headHPRecord = _headHPRecord;
    MARE_INTERNAL_ASSERT(
      headHPRecord->nrThreads()>0,
      "nrThreads should be > 0");
#ifdef _MSC_VER
    // VS2012 cannot create variable length arrays, so use an
    // std::vector instead
    std::vector<TN*> pList_temp (headHPRecord->nrThreads()*K);
    TN** pList = pList_temp.data();
#else
    // volatile?
    TN* pList[headHPRecord->nrThreads()*K];
#endif

    Record_p myRecord = _myhprec;

    TN** pIdx = pList;
    for(Record_p hprec = headHPRecord; hprec != nullptr;
        hprec = hprec->_next) {
      if(hprec == myRecord) {
        continue;
      }
      for(int i=0; i<K; ++i) {
        TN* hptr = hprec->_HP[i];
        if(hptr == node) {
          return true;
        }
      }
    }

    return false;
  }

private:
  static void cleanupTrampoline(void* manager) {
    static_cast<mare::internal::hp::Manager<TN,K>*>(manager)->
      threadFinishAuto();
  }

private:

  void allocRecord(tlsptr<Record<TN,K>>& myhprec) {
    Record_p hprec;
    Record_p oldHead;

    //std::cout << "[" << thread_id() << "] OLD myhprec " <<
    //  myhprec << std::endl;
    MARE_INTERNAL_ASSERT(!myhprec, "ERROR: myhprec was already initialized");

    // Try to reuse an inactive record
    for(hprec = _headHPRecord; hprec!=nullptr; hprec = hprec->next()) {
      if(!hprec->tryClaim()) continue;
      // Succeeded in locking an inactive HP record
      myhprec = hprec;
      //std::cout << "[" << thread_id() << "] myhprec " <<
      //  myhprec << std::endl;
      return;
    }

    // No HP records available for reuse
    // Increment _recordCount, then allocate a new Record and push it
    _recordCount++;
    // FIXME: investigate why the following:
    if(_recordCount >= MINIMAL_RLIST_SIZE/2) {
      _nodeThreshold = _recordCount*2;
    }
    hprec = new Record<TN,K>();
    if(!hprec->tryClaim()) {
      MARE_FATAL("Could not claim my own record");
      //print();
    }
    do { // wait-free - max. #threads is finite
      oldHead = _headHPRecord;
      hprec->next(oldHead);
      hprec->nrThreads(_recordCount);
    } while(!_headHPRecord.compare_exchange_strong(oldHead,hprec));
    myhprec = hprec;
    //std::cout << "[" << thread_id() << "] myhprec " <<
    //  myhprec << std::endl;
    return;
  }

  void retireNode(tlsptr<Record<TN,K>>& myhprec, TN* node) {
    MARE_INTERNAL_ASSERT(
      myhprec,
      "Please call threadInit() on each thread using hp::Manager");
    myhprec->push(node);
    Record_p head = _headHPRecord;
    if(myhprec->rCount() >= _nodeThreshold) {
      myhprec->scan(head);
      myhprec->helpScan(_headHPRecord,_nodeThreshold);
    }
  }

private:
  std::atomic<Record_p> _headHPRecord;
  std::atomic<int> _recordCount;
  int _nodeThreshold;
private:
  tlsptr<Record<TN,K>> _myhprec;
};

template<typename TN,int K>
const int Manager<TN,K>::MINIMAL_RLIST_SIZE = 10;

} //namespace hp

} //namespace internal

} //namespace mare
