/*
 * Copyright (C) 2013 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#define CATCH_CONFIG_MAIN

#include "entityx/3rdparty/catch.hpp"
#include "entityx/deps/Dependencies.h"
#include "entityx/quick.h"


namespace deps = entityx::deps;


struct A : public entityx::Component<A> {};
struct B : public entityx::Component<B> {
  explicit B(bool b = false) : b(b) {}

  bool b;
};
struct C : public entityx::Component<C> {};

TEST_CASE_METHOD(entityx::EntityX, "TestSingleDependency") {
  systems.add<deps::Dependency<A, B>>();
  systems.configure();

  entityx::Entity e = entities.create();
  REQUIRE(!static_cast<bool>(e.component<A>()));
  REQUIRE(!static_cast<bool>(e.component<B>()));
  e.assign<A>();
  REQUIRE(static_cast<bool>(e.component<A>()));
  REQUIRE(static_cast<bool>(e.component<B>()));
}

TEST_CASE_METHOD(entityx::EntityX, "TestMultipleDependencies") {
  systems.add<deps::Dependency<A, B, C>>();
  systems.configure();

  entityx::Entity e = entities.create();
  REQUIRE(!static_cast<bool>(e.component<A>()));
  REQUIRE(!static_cast<bool>(e.component<B>()));
  REQUIRE(!static_cast<bool>(e.component<C>()));
  e.assign<A>();
  REQUIRE(static_cast<bool>(e.component<A>()));
  REQUIRE(static_cast<bool>(e.component<B>()));
  REQUIRE(static_cast<bool>(e.component<C>()));
}

TEST_CASE_METHOD(entityx::EntityX, "TestDependencyDoesNotRecreateComponent") {
  systems.add<deps::Dependency<A, B>>();
  systems.configure();

  entityx::Entity e = entities.create();
  e.assign<B>(true);
  REQUIRE(e.component<B>()->b);
  e.assign<A>();
  REQUIRE(e.component<B>()->b);
}
