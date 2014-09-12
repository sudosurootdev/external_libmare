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

#include <cstring>
#include <list>
#include <mutex>

#include <mare/internal/macros.hh>
#include <mare/mutex.hh>

#include <mare/internal/sdf/channel.hh>

namespace mare {
namespace internal {

class sdf_node_common;

  /// char_array_buffer:
  /// character buffer storage for channel data

class char_array_buffer {
  std::size_t _allocated_num_elems;
  char* _buffer;
  std::size_t _buffer_num_bytes;
  std::size_t _head_index;
  std::size_t _stored_num_elems;

public:
  std::size_t const _elem_size;

  char_array_buffer(std::size_t elem_size);
  ~char_array_buffer();

  inline std::size_t get_allocated_num_elems() const
  {
    return _allocated_num_elems;
  }

  inline std::size_t get_stored_num_elems() const
  {
    return _stored_num_elems;
  }

  void resize(std::size_t new_allocated_num_elems);
  void read(char* pdata);
  void write(char const* pdata);

  MARE_DELETE_METHOD(char_array_buffer(char_array_buffer const&));
  MARE_DELETE_METHOD(char_array_buffer operator=(char_array_buffer const&));
  MARE_DELETE_METHOD(char_array_buffer(char_array_buffer&&));
  MARE_DELETE_METHOD(char_array_buffer operator=(char_array_buffer&&));
};

  /// locked_queue_buffer:
  /// queue interface on top of char_array_buffer.
  /// Boolean switches between locking and non-locking implementation.
  ///
  /// Semantics:
  /// - If read() or write() blocks due to channel empty/full respectively,
  ///   _read_was_blocked/_write_was_blocked is set for caller to detect.
  /// - If blocked, the caller must invoke trigger_read_resume() or
  ///   trigger_write_resume(), respectively, prior to re-trying the blocked
  ///   read or write, respectively, again.
  /// - A blocked read and a blocked write are independent events, and are
  ///   handled (signal the blockage, trigger resume, and retry later)
  ///   independent of each other.

class locked_queue_buffer {
  char_array_buffer       _char_array_buffer;

  bool                    _b_locked_access;
  std::mutex              _lock;


  /// interruption/resume flags. If for locked channel,
  ///    access only with _lock locked.
  bool _read_was_blocked;
  bool _write_was_blocked;
  bool _perform_read_resume;
  bool _perform_write_resume;

public:
  locked_queue_buffer(std::size_t elem_size, bool locked_access);
  ~locked_queue_buffer() {}

  inline std::size_t get_allocated_num_elems() const
  {
    return _char_array_buffer.get_allocated_num_elems();
  }

  inline std::size_t get_stored_num_elems() const
  {
    return _char_array_buffer.get_stored_num_elems();
  }

  inline void resize(std::size_t new_allocated_num_elems)
  {
    _char_array_buffer.resize(new_allocated_num_elems);
  }

  void trigger_read_resume();
  void trigger_write_resume();


  void read(char* pdata);
  void write(char const* pdata);

  inline bool get_read_was_blocked() const
  {
    return _read_was_blocked;
  }

  inline bool get_write_was_blocked() const
  {
    return _write_was_blocked;
  }

  MARE_DELETE_METHOD(locked_queue_buffer(
                                    locked_queue_buffer const&));
  MARE_DELETE_METHOD(locked_queue_buffer operator=(
                                    locked_queue_buffer const&));
  MARE_DELETE_METHOD(locked_queue_buffer(
                                    locked_queue_buffer&&));
  MARE_DELETE_METHOD(locked_queue_buffer operator=(
                                    locked_queue_buffer&&));
};

///////////////////////////////////////////////////////////////////

typedef locked_queue_buffer channel_buffer;

  /// channel_internal_record:
  /// provides the internal implementation of a channel, the parts
  /// hidden from the user.
class channel_internal_record {

  /// initializes the internal data for channels connecting to an SDF node
  friend void set_all_src_dst(
    sdf_node_common*                sdf_node,
    std::vector<direction> const&   vdir,
    std::vector<std::size_t> const& vsizes,
    std::vector<channel*> const&    vchannels);

  /// _src node pushes data into this channel
  sdf_node_common* _src;
  /// _dst node pops data from this channel
  sdf_node_common* _dst;

  /// sizeof(T) for data_channel<T>
  std::size_t _elem_size;

  /// indicates if channel has delay initializer data
  /// associated with it via preload_channel().
  bool        _b_preloaded_data;

  channel_buffer* _cb;

public:
  channel_internal_record();
  channel_internal_record(std::size_t elem_size);

  ~channel_internal_record();

  inline sdf_node_common* get_src() const
  {
    return const_cast<sdf_node_common *>(_src);
  }

  inline sdf_node_common* get_dst() const
  {
    return const_cast<sdf_node_common *>(_dst);
  }

  inline std::size_t get_elem_size() const { return _elem_size; }

  inline bool is_preloaded() const { return _b_preloaded_data; }

  inline channel_buffer* get_cb() const
  {
    return const_cast<channel_buffer*>(_cb);
  }

  /// Release the existing channel buffer
  inline channel_buffer* move_cb()
  {
    auto released_cb = _cb;
    _cb = nullptr;
    return released_cb;
  }

  void allocate_cb(std::size_t num_elems, bool locked_access)
  {
    MARE_INTERNAL_ASSERT(_cb == nullptr, "channel buffer already allocated");
    _cb = new channel_buffer(_elem_size, locked_access);
    _cb->resize(num_elems);
  }

  void preload_allocate(std::size_t num_elems);

  MARE_DELETE_METHOD(channel_internal_record(
                                        channel_internal_record const&));
  MARE_DELETE_METHOD(channel_internal_record operator=(
                                        channel_internal_record const&));
  MARE_DELETE_METHOD(channel_internal_record(
                                        channel_internal_record&&));
  MARE_DELETE_METHOD(channel_internal_record operator=(
                                        channel_internal_record&&));
};

  /// channel_accessor:
  /// a friend class to access internals from a user-visible abstract channel.

class channel_accessor {
public:
  static inline channel_internal_record* get_cir(channel const* c)
  {
    return const_cast<channel*>(c)->_cir;
  }
};


} //namespace internal
} //namespace mare
