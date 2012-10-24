/**
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 * 
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include <gtest/gtest.h>
#include "entityx/Components.h"

using namespace std;
using namespace boost;
using namespace entityx;


struct Position : public Component<Position> {};


template <typename T>
int size(const T &t) {
  int n = 0;
  for (auto i : t) {
    ++n;
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
  EventManager ev;
  EntityManager en(ev);
  Entity a = en.create();
  en.assign<Position>(a);
  for (int i = 0; i < 99; ++i) {
    auto e = en.create();
    en.assign<Position>(e);
    en.assign<TagsComponent>(e, "positionable");
  }
  en.assign<TagsComponent>(a, "player", "indestructible");
  auto entities = en.entities_with_components<Position>();
  ASSERT_EQ(100, size(entities));
  ASSERT_EQ(1, size(TagsComponent::view(entities, "player")));
}
