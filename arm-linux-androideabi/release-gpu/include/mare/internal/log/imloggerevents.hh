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

#include <thread>

#include <iostream>

#include <mare/internal/log/imlogger.hh>

namespace mare {
namespace internal {
namespace log {


// GCC moans about base class having a non-virtual destructor
MARE_GCC_IGNORE_BEGIN("-Weffc++");

// Little macro to save us a lot of typing
// Adds EVENT to EVENT to handler map
#define MARE_ADD_TO_MEMBUF_MAP(EVENT)                                   \
  template<>                                                            \
  struct event_handler_map<events::EVENT> {                             \
    typedef EVENT##_handler handler;                                    \
  };                                                                    \
                                                                        \

// Maps events to event handlers
template <typename EVENT>
struct event_handler_map;


struct event_handler_base {
  typedef seq_object_id<task> task_id;
  typedef seq_object_id<group> group_id;

  virtual std::string to_string() const = 0;
  virtual ~event_handler_base() {}

protected:
  static std::string get_task_name(task_id);
  static std::string get_group_name(group_id);
};


class ref_counted_event_handler : public event_handler_base {

  void* _object;
  const size_t _count;

protected:

  template<typename EVENT>
  ref_counted_event_handler(EVENT &&e) :
  _object(e.get_object()),
  _count(e.get_count()) { }

  void* get_object() const { return _object; }
  size_t get_count() const { return _count; }
};

class single_group_event_handler : public event_handler_base {

  group *_group;
  group_id _group_id;

protected:

  template<typename EVENT>
  single_group_event_handler(EVENT &&e):
  _group(e.get_group()),
  _group_id(_group->get_log_id()) { }

  group* get_group() const { return _group; }
  std::string get_group_name() const;
};


class single_task_event_handler : public event_handler_base {

  task *_task;
  task_id _task_id;

protected:

  template<typename EVENT>
  single_task_event_handler(EVENT &&e):
  _task(e.get_task()),
  _task_id(_task->get_log_id()) { }

  task* get_task() const { return _task; }
  std::string get_task_name() const;
};


class dual_task_event_handler : public single_task_event_handler {

  task *_other_task;
  task_id _other_task_id;

protected:

  template<typename EVENT>
  dual_task_event_handler(EVENT &&e) :
    single_task_event_handler(std::forward<EVENT>(e)),
    _other_task(e.get_other_task()),
    _other_task_id(_other_task->get_log_id()) { }

  task* get_other_task() const { return _other_task; }
  std::string get_other_task_name() const;
};


//
// Event Handlers. Keep them in alphabetical order.
//

struct group_canceled_handler : public single_group_event_handler {

  typedef events::group_canceled event;

  group_canceled_handler(event&& e) :
    single_group_event_handler(std::forward<event>(e)),
    _success(e.get_success()) { }

  std::string to_string() const;

private:

  bool _success;
};
MARE_ADD_TO_MEMBUF_MAP(group_canceled);


struct group_created_handler : public single_group_event_handler {

  typedef events::group_created event;

  group_created_handler(event&& e):
    single_group_event_handler(std::forward<event>(e)),
    _is_leaf(e.get_group()->is_leaf()) { }

  std::string to_string() const;

private:

  bool _is_leaf;
};
MARE_ADD_TO_MEMBUF_MAP(group_created);


struct group_destroyed_handler : public single_group_event_handler {

  typedef events::group_destroyed event;

  group_destroyed_handler(event&& e):
    single_group_event_handler(std::forward<event>(e)) {}

  std::string to_string() const;
};
MARE_ADD_TO_MEMBUF_MAP(group_destroyed);



struct group_reffed_handler : public single_group_event_handler {

  typedef events::group_reffed event;

  group_reffed_handler(event&& e):
    single_group_event_handler(std::forward<event>(e)),
    _count(e.get_count()) { }

  std::string to_string() const;

private:

  size_t _count;
};
MARE_ADD_TO_MEMBUF_MAP(group_reffed);


struct group_unreffed_handler : public single_group_event_handler {

  typedef events::group_unreffed event;

  group_unreffed_handler(event&& e):
    single_group_event_handler(std::forward<event>(e)),
    _count(e.get_count()) { }

  std::string to_string() const;

private:

  size_t _count;
};
MARE_ADD_TO_MEMBUF_MAP(group_unreffed);


struct object_reffed_handler : public ref_counted_event_handler {

  typedef events::object_reffed event;

  object_reffed_handler(event&& e):
    ref_counted_event_handler(std::forward<event>(e)){ }

  std::string to_string() const;
};
MARE_ADD_TO_MEMBUF_MAP(object_reffed);



struct object_unreffed_handler : public ref_counted_event_handler {

  typedef events::object_unreffed event;

  object_unreffed_handler(event&& e):
    ref_counted_event_handler(std::forward<event>(e)) {}

  std::string to_string() const;
};
MARE_ADD_TO_MEMBUF_MAP(object_unreffed);


struct task_after_handler : public dual_task_event_handler {

  typedef events::task_after event;

  task_after_handler(event&& e):
    dual_task_event_handler(std::forward<event>(e)){}

  std::string to_string() const;
};
MARE_ADD_TO_MEMBUF_MAP(task_after);


struct task_cleanup_handler : public single_task_event_handler {

  typedef events::task_cleanup event;

  task_cleanup_handler(event&& e):
    single_task_event_handler(std::forward<event>(e)),
    _canceled(e.get_canceled()),
    _in_utcache(e.get_in_utcache()) { }

  std::string to_string() const;

private:

  bool _canceled;
  bool _in_utcache;
};
MARE_ADD_TO_MEMBUF_MAP(task_cleanup);


struct task_created_handler : public single_task_event_handler {

  typedef events::task_created event;

  task_created_handler(event&& e):
    single_task_event_handler(std::forward<event>(e)),
    _attrs(e.get_task()->get_attrs()) { }

  std::string to_string() const;

private:
  task_attrs _attrs;
};
MARE_ADD_TO_MEMBUF_MAP(task_created);


struct task_destroyed_handler : public single_task_event_handler {

  typedef events::task_destroyed event;

  task_destroyed_handler(event&& e):
    single_task_event_handler(std::forward<event>(e)) {}

  std::string to_string() const;
};
MARE_ADD_TO_MEMBUF_MAP(task_destroyed);

struct task_done_handler : public single_task_event_handler {

  typedef events::task_done event;

  task_done_handler(event&& e):
    single_task_event_handler(std::forward<event>(e)) {}

  std::string to_string() const;
};
MARE_ADD_TO_MEMBUF_MAP(task_done);


struct task_executes_handler : public single_task_event_handler {

  typedef events::task_executes event;

  task_executes_handler(event&&e ) :
    single_task_event_handler(std::forward<event>(e)),
    _group(e.get_task()->get_group(std::memory_order_relaxed)),
    _group_id(_group? _group->get_log_id(): null_object_id<group>()),
    _runtime_owned(e.get_task()->is_runtime_owned()) { }

  std::string to_string() const;

  group* get_group() const { return _group; }

  bool is_runtime_owned() const { return _runtime_owned; }


private:

  std::string get_group_name() const {
    if (!_group)
      return "no group";
    return event_handler_base::get_group_name(_group_id);
  }

  group* _group;
  group_id _group_id;

  bool _runtime_owned;
};
MARE_ADD_TO_MEMBUF_MAP(task_executes);


struct task_sent_to_runtime_handler : public single_task_event_handler {

  typedef events::task_sent_to_runtime event;

  task_sent_to_runtime_handler(event&& e):
    single_task_event_handler(std::forward<event>(e)) {}

  std::string to_string() const;
};
MARE_ADD_TO_MEMBUF_MAP(task_sent_to_runtime);


struct task_reffed_handler : public single_task_event_handler {

  typedef events::task_reffed event;

  task_reffed_handler(event&& e):
    single_task_event_handler(std::forward<event>(e)),
    _count(e.get_count()) { }

  std::string to_string() const;

private:

  size_t _count;
};
MARE_ADD_TO_MEMBUF_MAP(task_reffed);


struct task_unreffed_handler : public single_task_event_handler {

  typedef events::task_unreffed event;

  task_unreffed_handler(event&& e):
    single_task_event_handler(std::forward<event>(e)),
    _count(e.get_count()) { }

  std::string to_string() const;

private:

  size_t _count;
};
MARE_ADD_TO_MEMBUF_MAP(task_unreffed);



struct task_wait_handler : public single_task_event_handler {

  typedef events::task_wait event;

  inline task_wait_handler(event&& e):
    single_task_event_handler(std::forward<event>(e)),
    _wait_required(e.get_wait_required())
  { }
  std::string to_string() const;

  const bool _wait_required;
};
MARE_ADD_TO_MEMBUF_MAP(task_wait);


struct runtime_disabled_handler : public event_handler_base {

  typedef events::runtime_disabled event;

  runtime_disabled_handler(event&&) {}

  std::string to_string() const;
};
MARE_ADD_TO_MEMBUF_MAP(runtime_disabled);


struct runtime_enabled_handler : public event_handler_base {

  typedef events::runtime_enabled event;

  runtime_enabled_handler(event&& e) :
    _num_exec_ctx(e.get_num_exec_ctx()) { }
  std::string to_string() const;

private:

  size_t _num_exec_ctx;
};
MARE_ADD_TO_MEMBUF_MAP(runtime_enabled);

struct unknown_event_handler : public event_handler_base {

  typedef events::unknown_event event;

  std::string to_string() const;

  template<typename EVENT>
  unknown_event_handler(EVENT &&e):
  _event_id(e.get_id()),
  _event_name(e.get_name()){ }

private:

  event_id _event_id;
  const char* _event_name;
};

template<typename EVENT>
struct event_handler_map {
  typedef unknown_event_handler handler;

};

template<typename Event>
inline void
imlogger::log(Event&& event, event_context& context)
{
  auto count = context.get_count();
  auto pos =  count % s_size;
  auto& entry = s_entries[pos];
  typedef typename event_handler_map<Event>::handler handler;

  static_assert(sizeof(handler) <= imlogger::entry::s_payload_size,
                "Handler is larger than entry payload.");

  entry.reset(count, handler::event::get_id(), context.get_this_thread_id());

  new (entry.get_buffer()) handler(std::forward<Event>(event));
}

#undef MARE_ADD_TO_MEMBUF_MAP

MARE_GCC_IGNORE_END("-Weffc++");

} // namespace mare::internal::log
} // namespace mare::internal
} // namespace mare

