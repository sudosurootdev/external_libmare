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

#include <mutex>
#include <vector>

#include <mare/internal/threadpool/storage.hh>

namespace mare {

namespace internal {

template <typename Item>
class locked_lifo : public storage_interface<Item> {
public:
  typedef typename storage_interface<Item>::item_t item_t;
  locked_lifo()
    : _items(),
      _mutex(),
      _cv()
  {};
  virtual ~locked_lifo() {}

  MARE_DELETE_METHOD(locked_lifo(locked_lifo const&));
  MARE_DELETE_METHOD(const locked_lifo& operator=(locked_lifo const&));

  void enqueue(item_t* item) {
    std::lock_guard<std::mutex> lock(_mutex);
    _items.push_back(item);
    if (_items.size() == 1) {
      _cv.notify_all();
    }
  }

  bool dequeue(item_t*& item) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_items.empty())
      return false;

    item = _items.back();
    _items.pop_back();
    return true;
  }

  item_t* dequeue() {
    std::unique_lock<std::mutex> lock(_mutex);
    while (_items.empty())
      _cv.wait(lock);

    auto item = _items.back();
    _items.pop_back();
    return item;
  }

  void wait() {
    std::unique_lock<std::mutex> lock(_mutex);
    while (_items.empty())
      _cv.wait(lock);
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _items.empty();
  }

  size_t get_size() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _items.size();
  }

protected:
  std::vector<item_t*> _items;
  mutable std::mutex _mutex;                    /// protects _items
  std::condition_variable _cv;          /// signals available items
};

}; // namespace internal

}; // namespace mare
