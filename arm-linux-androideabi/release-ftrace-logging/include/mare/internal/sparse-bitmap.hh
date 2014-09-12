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

#include <algorithm>
#include <bitset>
#include <initializer_list>
#include <set>

#include <mare/internal/debug.hh>

namespace mare {
namespace internal {

/**
  This class implements a sparse bitmap using linked lists with
  each node including a size_t bitmap and the position (index) of
  the current element in the link list.  As an example if size_t is
  64bit and if bits 0, 1 and 64 of a are set a = (_bitmap = 0x3,
  _pos = 0) -> (_bitmap = 0x1, __pos = 64) ->NULL
*/

class sparse_bitmap {

public:

  /** Type of hash values*/
  typedef size_t hash_t;


/** This internal class reprsents each chunk in the linked list */
private:
  class chunk {

  public:

    /** Type of each chunk's bmp */
    typedef size_t bitmap_t;

    /** bits available in each chunk */
    const static size_t s_num_bits = (sizeof(bitmap_t) << 3);

    bitmap_t _bitmap;
    size_t _pos;
    chunk* _next;

    chunk(bitmap_t bmp, size_t pos, chunk* n = nullptr):
      _bitmap(bmp),
      _pos(pos),
      _next(n) {}

    chunk():
      _bitmap(0),
      _pos(0),
      _next(nullptr) {}

    /** @brief This function is used to change the value of inlined chunk */
    void initialize(bitmap_t bmp, size_t pos, chunk* n = nullptr) {
      _bitmap = bmp;
      _pos = pos;
      _next = n;
    }

    MARE_DELETE_METHOD(chunk& operator=(chunk& chnk));
    MARE_DELETE_METHOD(chunk(chunk& chnk));
    ~chunk(){}

  }; // class chunk

private:

  // The first chunk will be inlined in the sparse_bitmap class and the rest
  // are in a linked list starting from the next pointer of the first chunk
  // The inlined first chunk
  chunk _chunks;
  // The hash value associated with the bitmap
  size_t _hash_value;


  /** @brief Rewrites the bitmap linked list using one single bitmap */
  void set_bitmap(chunk::bitmap_t value) {
    clear();
    _chunks.initialize(value, 0, nullptr);
    // This is a special case (_chunks.pos = 0 and _chunks._next = nullptr)
    // in which the hash value will be equal to the bitmap in the chunk
    // So I write it this way to save some performance. We can as well just
    // call compute_hash() instead.

    _hash_value = static_cast<hash_t>(value);
  }

public:

  typedef chunk::bitmap_t bitmap_t;

  sparse_bitmap():
    _chunks(),
    _hash_value(0) {}

  struct singleton_t {};
  static singleton_t const singleton;

  /** @brief Singleton bitmap (one-bit-bitmap) constructor */
  explicit sparse_bitmap(size_t value):
    _chunks(value, 0, nullptr),
    _hash_value(static_cast<hash_t>(value)) {
  }

  /** @brief Singleton bitmap (one-bit-bitmap) constructor */
  sparse_bitmap(size_t bit, singleton_t const&):
    _chunks(),
    _hash_value(0) {
    first_set(bit);
  }

  /** @brief List-based constructor */
  explicit sparse_bitmap(std::initializer_list<size_t> list):
    _chunks(),
    _hash_value(0){
    for (auto& index: list)
      set(index);
  }

  /** @brief move constructor */
  sparse_bitmap(sparse_bitmap&& other):
  _chunks(other._chunks._bitmap,
          other._chunks._pos,
          std::move(other._chunks._next)),
  _hash_value(other._hash_value) {
    //Making sure other is in a valid state
    other._chunks.initialize(0, 0, nullptr);
    other._hash_value = 0;
  }

  /** @brief copy constructor */
  sparse_bitmap(const sparse_bitmap& bm):
    _chunks(bm._chunks._bitmap, bm._chunks._pos, nullptr),
    _hash_value(bm._hash_value) {
    // Copy the whole linked list starting from _chunks._next
    chunk* copy_chunk = bm._chunks._next;
    chunk* new_chunk = nullptr;
    chunk* prev_chunk = nullptr;
    while (copy_chunk != nullptr) {
      new_chunk = new chunk(copy_chunk->_bitmap,
          copy_chunk->_pos, nullptr);
      if (_chunks._next == nullptr)
        _chunks._next = new_chunk;
      if (prev_chunk != nullptr) {
        prev_chunk->_next = new_chunk;
      }
      prev_chunk = new_chunk;
      copy_chunk = copy_chunk->_next;
    }
  }

  ~sparse_bitmap(){
    clear();
  }

  MARE_DELETE_METHOD(sparse_bitmap& operator=(const sparse_bitmap& bm));

  /** @brief Re-assigns using move semantics */
  sparse_bitmap& operator=(sparse_bitmap&& other) {
    clear();
    //Copy the chunk bitmap and pos,
    //Perform move semantic on _next linked list
    _chunks.initialize(other._chunks._bitmap,
        other._chunks._pos,
        std::move(other._chunks._next));
    _hash_value = other._hash_value;
    //Making sure other is in a valid state
    other._chunks.initialize(0, 0, nullptr);
    return *this;
  }

  sparse_bitmap operator|(const sparse_bitmap& bm) {
    sparse_bitmap result;
    result.set_union(*this, bm);
    return result;
  }

  /** @brief Sets the bit at a given index */
  bool set(size_t index);

  /** @brief Similar to set but assumes the bitmap is initially empty and
      also specializes the common case first */
  bool first_set(size_t index) {
    if (index < sparse_bitmap::chunk::s_num_bits) {
      // The most common case: setting the leave signatures
      _chunks._bitmap = one_bit_bitmap(index);
    }
    else {
      // Slow path
      set(index);
    }
    return true;
  }


  /** @brief Returns the hash value associated with the bitmap */
  hash_t get_hash_value() const {return _hash_value;};

  /** @brief Erase the bitmap chunks and reset the hash value */
  void clear() {
    //Walk the bitmap linked list (starting from _chunks._next) and erase
    chunk* current_chunk = _chunks._next;
    chunk* next_chunk = current_chunk;
    while(current_chunk) {
      current_chunk = current_chunk->_next;
      delete next_chunk;
      next_chunk = current_chunk;
    }
    _chunks._bitmap = 0;
    _chunks._pos = 0;
    _chunks._next = nullptr;
    _hash_value = 0;
  }

  /** @brief Returns the number of set bits */
  size_t popcount() const;

  /**
      @brief Returns true if one bit is set in the bitmap
      This is a more efficient implementation than calling popcount
      Maybe we can do it more efficiently using ffs or other mechanisms
  */
  bool is_singleton() const {
    //In a singleton we only have the first chunk and there is one bit in it
    return (_chunks._next == nullptr &&
        (mare_popcountw(_chunks._bitmap) == 1));
  };

  /**
        @brief Returns true if the bitmap is empty.
        The only thing we need to the check is the bitmap of the first chunk
    */
  bool is_empty() const {
    return (_chunks._bitmap == 0);
  }

  inline bitmap_t one_bit_bitmap(size_t index) {
    return (static_cast<bitmap_t>(1) << index);
  };

  /** @brief Inserts a bitmap chunk in the middle of two chunks */
  chunk* insert_chunk(chunk* prev, chunk* next, bitmap_t bmp, size_t new_pos);

  void recompute_hash();

  bool operator==(const sparse_bitmap& bm) const;

  /** @brief Combines two bitmaps and write the value in this object */
  void set_union(const sparse_bitmap& bm1, const sparse_bitmap& bm2);

  /** @brief Similar to set union but specializes the common case */
  void fast_set_union(const sparse_bitmap& bm1, const sparse_bitmap& bm2) {
    if (bm1._chunks._pos == 0 && bm1._chunks._next == nullptr &&
        bm2._chunks._pos == 0 && bm2._chunks._next == nullptr) {
      // Fast path: when both bitmaps are basic bitmap which means they
      // only occupy the first chunk
      _chunks._pos = 0;
      _chunks._next = nullptr;
      _chunks._bitmap = bm1._chunks._bitmap | bm2._chunks._bitmap;
      _hash_value = static_cast<hash_t>(_chunks._bitmap);
    }
    else {
      // Slow path
      set_union(bm1, bm2);
    }
  }

  /** @brief Returns true if bm is a subset of this object */
  bool subset(sparse_bitmap const& bm) const;

  std::string to_string() const;
};
};
};
