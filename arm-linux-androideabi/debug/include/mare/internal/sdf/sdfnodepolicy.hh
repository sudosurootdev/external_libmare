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

#include <mare/internal/sdf/sdfbasedefs.hh>
#include <mare/sdfpr.hh>

  /// SDF Node Policies
  ///
  /// An SDF node is compile-time parameterized along three dimensions.
  /// Policies are used to compose the node functionality.
  ///
  ///  1. Node function representation (node_fn_policy)
  ///     Provides a representation for the node function and a representation
  ///     of the channel-data values that the given node function
  ///     representation can conveniently/efficiently access.
  ///      - a pure function     (node_fn_policy__func_pointer)
  ///      - function with state (node_fn_policy__std_function)
  ///      - programmatic introspection of connected channels
  ///                            (node_fn_policy__programatic_function)
  ///
  ///  2. Channel representation       (channel_repr_policy)
  ///      - handles to the connected typed-channels (i.e., data_channel<T>)
  ///                            (channel_repr_policy__data_channel)
  ///      - handles to the connected untyped channels with element-size
  ///         information (channel *), but no compile-time type-safety.
  ///         Allows programmatic attachment of arbitrary number of channels,
  ///         of arbitrary user-data-types.
  ///                            (channel_repr_policy__sized_channel)
  ///
  ///  3. Channel initialization       (channel_init_policy)
  ///      Allows the mixing of any node_fn_policy with any
  ///      channel_repr_policy, by defining a common intermediate data-type
  ///      compatible with the given policy choices for both node_fn_policy
  ///      and channel_repr_policy.
  ///
  ///      - typed representation, when both policies carry user-data-types
  ///                            (channel_init_policy__data_channel_tuple)
  ///      - untyped representation, when at least one policy is untyped
  ///                            (channel_init_policy__channel_vector)


  /// node_fn_policy implementations
  ///  Components of the policy:
  ///    - ftype
  ///    - valtype

namespace mare {
namespace internal {

template<typename ...Ts>
struct node_fn_policy__func_pointer {
  typedef void (* ftype)(Ts&...);
  typedef std::tuple<Ts...> valtype;
};

////

template<typename ...Ts>
struct node_fn_policy__std_function {
  typedef std::function<void(Ts&...)> ftype;
  typedef std::tuple<Ts...> valtype;
};

} //namespace internal
} //namespace mare

////

namespace mare {

inline std::size_t
node_channels::get_num_channels() const
{
  return _vdir.size();
}

inline bool
node_channels::is_in_channel(std::size_t channel_index) const
{
  MARE_API_ASSERT(channel_index < _vdir.size(),
      "channel_index=%lld exceeds number of channels=%lld",
      static_cast<long long int>(channel_index),
      static_cast<long long int>(_vdir.size()));
  return (_vdir[channel_index] == internal::direction::in);
}

inline bool
node_channels::is_out_channel(std::size_t channel_index) const
{
  MARE_API_ASSERT(channel_index < _vdir.size(),
      "channel_index=%lld exceeds number of channels=%lld",
      static_cast<long long int>(channel_index),
      static_cast<long long int>(_vdir.size()));
  return (_vdir[channel_index] == internal::direction::out);
}

inline std::size_t
node_channels::get_elemsize(std::size_t channel_index) const
{
  MARE_API_ASSERT(channel_index < _vdir.size(),
      "channel_index=%lld exceeds number of channels=%lld",
      static_cast<long long int>(channel_index),
      static_cast<long long int>(_vdir.size()));
  return _velemsize[channel_index];
}

template<typename T>
void
node_channels::read(T& t, std::size_t channel_index)
{
  MARE_API_ASSERT(channel_index < _vdir.size(),
      "channel_index=%lld exceeds number of channels=%lld",
      static_cast<long long int>(channel_index),
      static_cast<long long int>(_vdir.size()));
  MARE_API_ASSERT(_vdir.at(channel_index) == internal::direction::in,
      "channel at index=%lld is not an in-channel.",
      static_cast<long long int>(channel_index));
  MARE_API_ASSERT(sizeof(t) == _velemsize.at(channel_index),
      "Variable sizeof mismatch on reading channel index=%lld",
      static_cast<long long int>(channel_index));
  std::memcpy(
              reinterpret_cast<void*>(&t),
              reinterpret_cast<void const*>(_vbuffer[channel_index]),
              sizeof(t));
}

template<typename T>
void
node_channels::write(T const& t, std::size_t channel_index)
{
  MARE_API_ASSERT(channel_index < _vdir.size(),
      "channel_index=%lld exceeds number of channels=%lld",
      static_cast<long long int>(channel_index),
      static_cast<long long int>(_vdir.size()));
  MARE_API_ASSERT(_vdir.at(channel_index) == internal::direction::out,
      "channel at index=%lld is not an out-channel.",
      static_cast<long long int>(channel_index));
  MARE_API_ASSERT(sizeof(t) == _velemsize.at(channel_index),
      "Variable sizeof mismatch on writing channel index=%lld",
      static_cast<long long int>(channel_index));
  std::memcpy(
              reinterpret_cast<void*>(_vbuffer[channel_index]),
              reinterpret_cast<void const*>(&t),
              sizeof(t));
}

} //namespace mare

namespace mare {
namespace internal {

class node_channels_accessor {
public:
  static std::vector<direction>& access_vdir(node_channels& ncs)
  {
    return ncs._vdir;
  }

  static std::vector<std::size_t>& access_velemsize(node_channels& ncs)
  {
    return ncs._velemsize;
  }

  static std::vector<char*>& access_vbuffer(node_channels& ncs)
  {
    return ncs._vbuffer;
  }
};

template<typename ...Ts>
struct node_fn_policy__programatic_function {
  typedef void (* ftype)(node_channels&);
  typedef std::tuple<node_channels> valtype;
};

} //namespace internal
} //namespace mare


  /// channel_repr_policy implementations
  ///  Components of the policy:
  ///    - channel_repr
  ///      - channel_handles
  ///      - init()

namespace mare {
namespace internal {

template<typename ...Ts>
struct data_channel_parameters {
  std::tuple< data_channel<Ts>*... > _channel_handles;

  data_channel_parameters() :
    _channel_handles() { }

  std::vector<channel*> init(std::tuple< data_channel<Ts>*...>& pdc_tuple)
  {
    _channel_handles = pdc_tuple;
    return map_data_channel_tuple_to_channel_ptr(pdc_tuple);
  }
};

template<typename ...Ts>
struct channel_repr_policy__data_channel {
  typedef data_channel_parameters<Ts...> channel_repr;
};

////

template<typename ...Ts>
class sized_channel_parameters {
public:
  std::vector<channel*>   _channel_handles;

  std::vector<std::size_t> _velemsize;
  std::vector<char*>      _vbuffer;

  sized_channel_parameters() :
    _channel_handles(),
    _velemsize(),
    _vbuffer() { }

  std::vector<channel*> init(std::tuple< data_channel<Ts>*...>& pdc_tuple)
  {
    _channel_handles = map_data_channel_tuple_to_channel_ptr(pdc_tuple);
    setup();
    return _channel_handles;
  }

  std::vector<channel*> init(std::vector<channel*>& vchannels)
  {
    _channel_handles = vchannels;
    setup();
    return _channel_handles;
  }

  ~sized_channel_parameters()
  {
    for(auto& buf : _vbuffer) {
      MARE_INTERNAL_ASSERT(buf != 0, "buffer should still have existed");
      delete[] buf;
      buf = 0;
    }
  }

private:
  void setup()
  {
    _velemsize.resize(_channel_handles.size());
    _vbuffer.resize(_channel_handles.size());
    for(std::size_t i=0; i<_channel_handles.size(); i++) {
      _velemsize[i] = _channel_handles[i]->get_elem_size();
      _vbuffer[i] = new char[ _velemsize[i] ];
    }
  }

};

template<typename ...Ts>
struct channel_repr_policy__sized_channel {
  typedef sized_channel_parameters<Ts...> channel_repr;
};

} //namespace internal
} //namespace mare


  /// channel_init_policy implementations
  ///  Components of the policy:
  ///    - init_type

namespace mare {
namespace internal {

template<typename ...Ts>
struct channel_init_policy__data_channel_tuple {
  typedef std::tuple< data_channel<Ts>*...> init_type;
};

template<typename ...Ts>
struct channel_init_policy__channel_vector {
  typedef std::vector<channel*> init_type;
};

} //namespace internal
} //namespace mare
