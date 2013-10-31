/*
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include <gtest/gtest.h>
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
    (void)i; // Unused on purpose, suppress warning
  }
  return n;
}


TEST(TagsComponentTest, TestVariadicConstruction) {
  auto tags = TagsComponent("player", "indestructible");
  unordered_set<string> expected;
  expected.insert("player");
  expected.insert("indestructible");
  ASSERT_TRUE(expected == tags.tags);
}

TEST(TagsComponentTest, TestEntitiesWithTag) {
  auto en = EntityManager::make(EventManager::make());
  Entity a = en->create();
  a.assign<Position>();
  for (int i = 0; i < 99; ++i) {
    auto e = en->create();
    e.assign<Position>();
    e.assign<TagsComponent>("positionable");
  }
  a.assign<TagsComponent>("player", "indestructible");
  auto entities = en->entities_with_components<Position>();
  ASSERT_EQ(100, size(entities));
  ASSERT_EQ(1, size(TagsComponent::view(entities, "player")));
}
