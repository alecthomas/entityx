/*
 * Copyright (C) 2013 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include <gtest/gtest.h>
#include "entityx/deps/Dependencies.h"


namespace ex = entityx;
namespace deps = entityx::deps;


struct A : public ex::Component<A> {};
struct B : public ex::Component<B> {
  explicit B(bool b = false) : b(b) {}

  bool b;
};
struct C : public ex::Component<C> {};


class DepsTest : public ::testing::Test {
 protected:
  DepsTest() : events(ex::EventManager::make()),
               entities(ex::EntityManager::make(events)),
               systems(ex::SystemManager::make(entities, events)) {}

  ex::ptr<ex::EventManager> events;
  ex::ptr<ex::EntityManager> entities;
  ex::ptr<ex::SystemManager> systems;

  virtual void SetUp() {}
};


TEST_F(DepsTest, TestSingleDependency) {
  systems->add<deps::Dependency<A, B>>();
  systems->configure();

  ex::Entity e = entities->create();
  ASSERT_FALSE(static_cast<bool>(e.component<A>()));
  ASSERT_FALSE(static_cast<bool>(e.component<B>()));
  e.assign<A>();
  ASSERT_TRUE(static_cast<bool>(e.component<A>()));
  ASSERT_TRUE(static_cast<bool>(e.component<B>()));
}

TEST_F(DepsTest, TestMultipleDependencies) {
  systems->add<deps::Dependency<A, B, C>>();
  systems->configure();

  ex::Entity e = entities->create();
  ASSERT_FALSE(static_cast<bool>(e.component<A>()));
  ASSERT_FALSE(static_cast<bool>(e.component<B>()));
  ASSERT_FALSE(static_cast<bool>(e.component<C>()));
  e.assign<A>();
  ASSERT_TRUE(static_cast<bool>(e.component<A>()));
  ASSERT_TRUE(static_cast<bool>(e.component<B>()));
  ASSERT_TRUE(static_cast<bool>(e.component<C>()));
}

TEST_F(DepsTest, TestDependencyDoesNotRecreateComponent) {
  systems->add<deps::Dependency<A, B>>();
  systems->configure();

  ex::Entity e = entities->create();
  e.assign<B>(true);
  ASSERT_TRUE(e.component<B>()->b);
  e.assign<A>();
  ASSERT_TRUE(e.component<B>()->b);
}
