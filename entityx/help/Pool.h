/*
 * Copyright (C) 2012-2014 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#pragma once

#include <cstddef>
#include <cassert>
#include <vector>

namespace entityx {

/**
 * Provides a resizable, semi-contiguous pool of memory for constructing
 * objects in. Pointers into the pool will be invalided only when the pool is
 * destroyed.
 *
 * The semi-contiguous nature aims to provide cache-friendly iteration.
 *
 * Lookups are O(1).
 * Appends are amortized O(1).
 */
class BasePool {
 public:
  explicit BasePool(std::size_t element_size, std::size_t chunk_size = 8192)
      : element_size_(element_size), chunk_size_(chunk_size), capacity_(0) {}
  virtual ~BasePool();

  std::size_t size() const { return size_; }
  std::size_t capacity() const { return capacity_; }
  std::size_t chunks() const { return blocks_.size(); }

  /// Ensure at least n elements will fit in the pool.
  inline void expand(std::size_t n) {
    if (n >= size_) {
      if (n >= capacity_) reserve(n);
      size_ = n;
    }
  }

  inline void reserve(std::size_t n) {
    while (capacity_ < n) {
      char *chunk = new char[element_size_ * chunk_size_];
      blocks_.push_back(chunk);
      capacity_ += chunk_size_;
    }
  }

  inline void *get(std::size_t n) {
    assert(n < size_);
    return blocks_[n / chunk_size_] + (n % chunk_size_) * element_size_;
  }

  inline const void *get(std::size_t n) const {
    assert(n < size_);
    return blocks_[n / chunk_size_] + (n % chunk_size_) * element_size_;
  }

  virtual void destroy(std::size_t n) = 0;

 protected:
  std::vector<char *> blocks_;
  std::size_t element_size_;
  std::size_t chunk_size_;
  std::size_t size_ = 0;
  std::size_t capacity_;
};


/**
 * Implementation of BasePool that provides type-"safe" deconstruction of
 * elements in the pool.
 */
template <typename T, std::size_t ChunkSize = 8192>
class Pool : public BasePool {
 public:
  Pool() : BasePool(sizeof(T), ChunkSize) {}
  virtual ~Pool() {
    // Component destructors *must* be called by owner.
  }

  virtual void destroy(std::size_t n) override {
    assert(n < size_);
    T *ptr = static_cast<T*>(get(n));
    ptr->~T();
  }
};

}  // namespace entityx
