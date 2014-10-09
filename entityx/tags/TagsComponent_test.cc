/*
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#define CATCH_CONFIG_MAIN

#include <string>
#include "entityx/3rdparty/catch.hpp"
#include "entityx/tags/TagsComponent.h"

using namespace std;
using namespace entityx;
using namespace entityx::tags;


struct Position : public Component<Position> {};


template <typename T>
int size(const T &t) {
  int n = 0;
  for (auto i : t) {
    ++n;
    (void)i;  // Unused on purpose, suppress warning
  }
  return n;
}


TEST_CASE("TestVariadicConstruction", "TagsComponentTest") {
  auto tags = TagsComponent("player", "indestructible");
  unordered_set<string> expected;
  expected.insert("player");
  expected.insert("indestructible");
  REQUIRE(expected == tags.tags);
}
