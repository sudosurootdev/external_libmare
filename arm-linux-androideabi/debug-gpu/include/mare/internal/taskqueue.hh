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

// For debug we can force a single Q that should perform the same a
// the global queue
//#define MARE_TQ_SINGLE _foreign_qidx()

// set to 1 to make sure that the dump method below is accessable from gdb.
#define MARE_TQ_DUMP 0

#if MARE_TQ_DUMP
#define MARE_TQ_FORCE MARE_GCC_ATTRIBUTE((used, noinline))
#else
#define MARE_TQ_FORCE
#endif

#include <atomic>

#include <mare/common.hh>
#include <mare/internal/task.hh>
#include <mare/internal/task_queue_adaptor.hh>
#include <mare/internal/thread_manager.hh>
#include <mare/internal/dealer.hh>
#include <mare/internal/tls.hh>

namespace mare
{

namespace internal
{

// Make these to be whatever you want.
typedef local_cld_task_queue main_task_queue;
typedef local_cld_task_queue local_task_queue;
typedef local_std_task_queue foreign_task_queue;

typedef thread_manager_condvar thread_manager;

class task_queue_list;
class device_thread;

class task_queue {
public:
  task_queue(local_task_queue& ltq, task_queue_list& tql,
             size_t qidx, dealer& d) :
    _ltq(ltq),
    _tql(tql),
    _qidx(qidx),
    _fuzzy(_ltq.element_count_is_fuzzy()),
    _dealer(std::move(d)),
    _last_card()
  {
    _dealer.shuffle();
  }
  MARE_DEFAULT_METHOD(~task_queue());

  size_t push(task* t) { return _ltq.push(t, task_queue_notify::NONE); }

  template <typename PREDICATE>
  size_t pop_if(PREDICATE pred, task*& t, paired_mutex_cv* synch) {
    size_t sz = _ltq.pop(t, synch);
    if (sz) {
      notify(sz - 1);
      return sz;
    }

    return steal_from_tql(pred, t, synch);
  }

  void notify_all(paired_mutex_cv& synch);
  void notify(size_t sz);
  void threadInitAuto() { _ltq.threadInitAuto(); }
  template <typename PREDICATE>
  size_t steal_from_tql(PREDICATE pred, task*& t, paired_mutex_cv* synch);
  size_t get_qidx() const { return _qidx; }
  bool fuzzy() const { return _fuzzy; }
  std::vector<size_t>& get_hand() {
    return _dealer.get_hand();
  }
  size_t get_last_card() { return _last_card; }
  void set_last_card(size_t last) { _last_card = last; }

private:
  local_task_queue& _ltq;
  task_queue_list& _tql;
  size_t _qidx;
  const bool _fuzzy;
  dealer _dealer;
  size_t _last_card;
};

class task_queue_list {

public:
  task_queue_list(size_t ex_con) :
    // We need a queue per execution context +1 for the main thread
    // and +1 for the forieng threads queue
    _qs(ex_con + 2),
    _main_q(),
    _foreign_q(),
    _tql(ex_con),
    _alloc_qidx(),
    _tm(ex_con)
  {
    // check
    MARE_INTERNAL_ASSERT(_qs > 2, "no local queues specificied");
    mare_constexpr_static_assert(foreign_task_queue::pop_is_mt_safe(),
                                 "foreign thread queue must be MT safe for "
                                 "pop and push");

    for (auto it = _tql.begin(); it != _tql.end(); it++) {
      *it = new local_task_queue;
    }
  }

  ~task_queue_list() {
    for (auto it = _tql.begin(); it != _tql.end(); it++) {
      delete *it;
    }
  }

  task_queue& get_queue();
  task_queue* current_task_queue(task *);
  static void put_queue(task_queue* tq) {
    if (tq != nullptr)
      delete tq;
  }
  size_t push_task_select(task* task);
  size_t steal_from_qidx(size_t qidx, task*& t, paired_mutex_cv* synch);
  inline void dump_sizes() MARE_TQ_FORCE;

  void push_task(task* task) { push_task_select(task); }
  void notify_all(paired_mutex_cv&) { _tm.notify_all(); }
  void notify(size_t qidx, size_t nelem, bool fuzzy) {
    _tm.notify(_q2lq(qidx), nelem, fuzzy);
  }

  template <typename PREDICATE>
  size_t steal_from_another(PREDICATE pred, task_queue& tq, task*& t,
                            paired_mutex_cv* synch);

private:
  size_t _qs;
  static constexpr size_t _foreign_qidx() { return 0; }
  static constexpr size_t _main_qidx() { return 1; }
  static constexpr size_t _q2lq(size_t qidx) { return qidx - 2; }
  static constexpr size_t _lq2q(size_t lqidx) { return lqidx + 2; }

  main_task_queue _main_q;
  foreign_task_queue _foreign_q;

  std::vector<local_task_queue*> _tql;

  /// eventually be a bitmap
  size_t _alloc_qidx;

  thread_manager _tm;
};

inline task_queue&
task_queue_list::get_queue()
{
  // This is currently not parallel, but could be in the future.
  // we will switch to a bitmap by then
  auto qi = _alloc_qidx;

#ifdef MARE_TQ_SINGLE
  // over-ride with a single queue
  qi = MARE_TQ_SINGLE;
#endif

  auto q = _tql[qi];

  // we still want to avoid the foreign thread queue and ourselves in
  // the deck
  dealer d(_main_qidx(), _qs - 2, _lq2q(qi));

  // dealer will now be "owned" by task_queue
  auto tq = new task_queue(*q, *this, _lq2q(qi), d);

  ++_alloc_qidx;
  return *tq;
}

inline size_t
task_queue_list::steal_from_qidx(size_t qidx, task*& t, paired_mutex_cv* synch)
{
  if (qidx == _main_qidx())
    return _main_q.steal(t, synch);
  if (qidx == _foreign_qidx())
    return _foreign_q.steal(t, synch);

  MARE_INTERNAL_ASSERT(qidx < _qs, "bad Q index");

  auto q = _q2lq(qidx);
  return _tql[q]->steal(t, synch);
}

template <typename PREDICATE>
size_t
task_queue_list::steal_from_another(PREDICATE pred, task_queue& tq, task*& t,
                                    paired_mutex_cv* synch)
{
  size_t sz;
  auto hand = tq.get_hand();
  auto last = tq.get_last_card();

  for (;;) {
    if (!pred())
      return 0;

    // this include the global queue somewhere
    for (auto q = last; q < last + hand.size(); q++) {
      auto qm = q % hand.size();
      sz = steal_from_qidx(hand[qm], t, synch);
      if (sz) {
        // remember the last good one
        tq.set_last_card(qm);
        return sz;
      }
    }
    // check foreign queue
    sz = steal_from_qidx(_foreign_qidx(), t, synch);
    if (sz)
      return sz;

    _tm.idle(pred, _q2lq(tq.get_qidx()));
  }
}

inline size_t
task_queue_list::push_task_select(task* task)
{
  bool fuzzy;
  size_t qidx;
  size_t el;

  if (task->is_yield()) {
    qidx = _foreign_qidx();
    fuzzy = _foreign_q.element_count_is_fuzzy();

    el = _foreign_q.push(task);

    _tm.notify(_q2lq(qidx), el, fuzzy);

    return el;
  }

  auto info = tls::get();

  if (info->is_main_thread()) {
    qidx = _main_qidx();
    fuzzy = _main_q.element_count_is_fuzzy();

    el = _main_q.push(task);

    _tm.notify(_q2lq(qidx), el, fuzzy);

    return el;
  }


  auto ct = info->current_task();
  if (ct) {
    auto tq = current_task_queue(ct);
    if (tq) {
      fuzzy = tq->fuzzy();
      qidx = tq->get_qidx();
      el = tq->push(task);
      _tm.notify(_q2lq(qidx), el, fuzzy);
      return el;
    }
  }
  qidx = _foreign_qidx();
  fuzzy = _foreign_q.element_count_is_fuzzy();

  el = _foreign_q.push(task);

  _tm.notify(_q2lq(qidx), el, fuzzy);

  return el;
}

inline void
task_queue::notify_all(paired_mutex_cv& synch)
{
  _tql.notify_all(synch);
}

inline void
task_queue::notify(size_t el)
{
  _tql.notify(_qidx, el, _fuzzy);
}

template <typename PREDICATE>
inline size_t task_queue::steal_from_tql(PREDICATE pred, task *& t,
                                         paired_mutex_cv* synch)
{
  return _tql.steal_from_another(pred, *this, t, synch);
}

inline void
task_queue_list::dump_sizes()
{
  MARE_ALOG("TQ: yy[%zu]: %zu\n", _foreign_qidx(), _foreign_q.unsafe_size());
  MARE_ALOG("TQ: gq[%zu]: %zu\n", _main_qidx(), _main_q.unsafe_size());
  size_t i = _lq2q(0);
  for (auto it = _tql.begin(); it != _tql.end(); it++) {
    auto q = *it;
    MARE_ALOG("TQ: lq[%zu]: %zu\n", i, q->unsafe_size());
    ++i;
  }
}

} //namespace internal

} //namespace mare
