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

#include <cassert>
#include <cstdint>
#include <functional>
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
  explicit BasePool(int element_size, int chunk_size = 8192)
      : element_size_(element_size), chunk_size_(chunk_size), capacity_(0) {}
  virtual ~BasePool();

  int size() const { return size_; }
  int capacity() const { return capacity_; }
  int chunks() const { return blocks_.size(); }

  /// Ensure at least n elements will fit in the pool.
  inline void expand(int n) {
    if (n >= size_) {
      if (n >= capacity_) reserve(n);
      size_ = n;
    }
  }

  inline void reserve(int n) {
    while (capacity_ < n) {
      char *chunk = new char[element_size_ * chunk_size_];
      blocks_.push_back(chunk);
      capacity_ += chunk_size_;
    }
  }

  inline void *get(int n) {
    assert(n >= 0 && n < size_);
    return blocks_[n / chunk_size_] + (n % chunk_size_) * element_size_;
  }

  inline const void *get(int n) const {
    assert(n >= 0 && n < size_);
    return blocks_[n / chunk_size_] + (n % chunk_size_) * element_size_;
  }

  virtual void destroy(int n) = 0;
  virtual void copy(int from, int to) = 0;

 protected:
  std::vector<char *> blocks_;
  int element_size_;
  int chunk_size_;
  int size_ = 0;
  int capacity_;
};


/**
 * Implementation of BasePool that provides type-"safe" deconstruction of
 * elements in the pool.
 */
template <typename T, int ChunkSize = 8192>
class Pool : public BasePool {
 public:
  Pool(std::function<void(uint32_t)> added_component_callback) : BasePool(sizeof(T), ChunkSize), added_component_callback_(added_component_callback) {}
  virtual ~Pool() {}

  virtual void destroy(int n) override {
    assert(n >= 0 && n < size_);
    T *ptr = static_cast<T*>(get(n));
    ptr->~T();
  }

  // Copy (via copy constructor) a component and call added_component_callback callback.
  virtual void copy(int from, int to) override {
    assert(from >= 0 && from < size_);
    assert(to >= 0 && to < size_);
    T *from_ptr = static_cast<T*>(get(from));
    new(static_cast<T*>(get(to))) T(*from_ptr);
    added_component_callback_(static_cast<uint32_t>(to));
  }

private:
  std::function<void(uint32_t)> added_component_callback_;
};

}  // namespace entityx
