/*
 * Copyright (C) 2012-2014 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#define CATCH_CONFIG_MAIN

#include <vector>
#include "entityx/3rdparty/catch.hpp"
#include "entityx/help/Pool.h"

struct Position {
  explicit Position(int *ptr = nullptr) : ptr(ptr) {
    if (ptr) (*ptr)++;
  }
  Position(const Position &copy) : ptr(copy.ptr) {
    if (ptr) (*ptr)++;
    x = copy.x;
    y = copy.y;
  }
  ~Position() {
    if (ptr) (*ptr)++;
  }

  float x, y;
  int *ptr;
};

void emit_added(uint32_t index) {}

TEST_CASE("TestPoolReserve") {
  entityx::Pool<Position, 8> pool(emit_added);
  REQUIRE(0 ==  pool.capacity());
  REQUIRE(0 ==  pool.chunks());
  pool.reserve(8);
  REQUIRE(0 ==  pool.size());
  REQUIRE(8 ==  pool.capacity());
  REQUIRE(1 ==  pool.chunks());
  pool.reserve(16);
  REQUIRE(0 ==  pool.size());
  REQUIRE(16 ==  pool.capacity());
  REQUIRE(2 ==  pool.chunks());
}

TEST_CASE("TestPoolPointers") {
  entityx::Pool<Position, 8> pool(emit_added);
  std::vector<char*> ptrs;
  for (int i = 0; i < 4; i++) {
    pool.expand(i * 8 + 8);
    // NOTE: This is an attempt to ensure non-contiguous allocations from
    // arena allocators.
    ptrs.push_back(new char[8 * sizeof(Position)]);
  }
  char *p0 = static_cast<char*>(pool.get(0));
  char *p7 = static_cast<char*>(pool.get(7));
  char *p8 = static_cast<char*>(pool.get(8));
  char *p16 = static_cast<char*>(pool.get(16));
  char *p24 = static_cast<char*>(pool.get(24));

  void *expected_p7 = p0 + 7 * sizeof(Position);
  REQUIRE(expected_p7 == static_cast<void*>(p7));
  void *extrapolated_p8 = p0 + 8 * sizeof(Position);
  REQUIRE(extrapolated_p8 !=  static_cast<void*>(p8));
  void *extrapolated_p16 = p8 + 8 * sizeof(Position);
  REQUIRE(extrapolated_p16 !=  static_cast<void*>(p16));
  void *extrapolated_p24 = p16 + 8 * sizeof(Position);
  REQUIRE(extrapolated_p24 !=  static_cast<void*>(p24));
}

TEST_CASE("TestDeconstruct") {
  entityx::Pool<Position, 8> pool(emit_added);
  pool.expand(8);

  void *p0 = pool.get(0);

  int counter = 0;
  new(p0) Position(&counter);
  REQUIRE(1 ==  counter);
  pool.destroy(0);
  REQUIRE(2 ==  counter);
}

TEST_CASE("TestCopy") {
  int added_event_called = 0;
  entityx::Pool<Position, 8> pool([&](uint32_t) { added_event_called++; });
  pool.expand(8);

  void *p0 = pool.get(0);

  int counter = 0;
  Position *pos0 = new(p0) Position(&counter);
  pos0->x = 1.0;
  pos0->y = 2.0;

  REQUIRE(1 ==  counter);

  pool.copy(0, 1);
  REQUIRE(2 ==  counter);
  Position *pos1 = static_cast<Position*>(pool.get(1));
  REQUIRE(pos0->x == pos1->x);
  REQUIRE(pos0->y == pos1->y);

  REQUIRE(added_event_called == 1);
}
