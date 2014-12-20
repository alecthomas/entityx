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
#include <vector>
#include "entityx/3rdparty/catch.hpp"
#include "entityx/System.h"
#include "entityx/quick.h"

// using namespace std;
using namespace entityx;
using std::string;

struct Position : Component<Position> {
  explicit Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  float x, y;
};

struct Direction : Component<Direction> {
  explicit Direction(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  float x, y;
};

struct Counter : Component<Counter> {
  explicit Counter(int counter = 0) : counter(counter) {}

  int counter;
};

class MovementSystem : public System<MovementSystem> {
 public:
  explicit MovementSystem(string label = "") : label(label) {}

  void update(EntityManager &es, EventManager &events, TimeDelta) override {
    EntityManager::View entities =
        es.entities_with_components<Position, Direction>();
    ComponentHandle<Position> position;
    ComponentHandle<Direction> direction;
    for (auto entity : entities) {
      entity.unpack<Position, Direction>(position, direction);
      position->x += direction->x;
      position->y += direction->y;
    }
  }

  string label;
};

class CounterSystem : public System<CounterSystem> {
public:
  void update(EntityManager &es, EventManager &events, TimeDelta) override {
    EntityManager::View entities =
        es.entities_with_components<Counter>();
    Counter::Handle counter;
    for (auto entity : entities) {
      entity.unpack<Counter>(counter);
      counter->counter++;
    }
  }
};

class EntitiesFixture : public EntityX {
 public:
  std::vector<Entity> created_entities;

  EntitiesFixture() {
    for (int i = 0; i < 150; ++i) {
      Entity e = entities.create();
      created_entities.push_back(e);
      if (i % 2 == 0) e.assign<Position>(1, 2);
      if (i % 3 == 0) e.assign<Direction>(1, 1);

      e.assign<Counter>(0);
    }
  }
};

TEST_CASE_METHOD(EntitiesFixture, "TestConstructSystemWithArgs") {
  systems.add<MovementSystem>("movement");
  systems.configure();

  REQUIRE("movement" == systems.system<MovementSystem>()->label);
}

TEST_CASE_METHOD(EntitiesFixture, "TestApplySystem") {
  systems.add<MovementSystem>();
  systems.configure();

  systems.update<MovementSystem>(0.0);
  ComponentHandle<Position> position;
  ComponentHandle<Direction> direction;
  for (auto entity : created_entities) {
    entity.unpack<Position, Direction>(position, direction);
    if (position && direction) {
      REQUIRE(2.0 == Approx(position->x));
      REQUIRE(3.0 == Approx(position->y));
    } else if (position) {
      REQUIRE(1.0 == Approx(position->x));
      REQUIRE(2.0 == Approx(position->y));
    }
  }
}

TEST_CASE_METHOD(EntitiesFixture, "TestApplyAllSystems") {
  systems.add<MovementSystem>();
  systems.add<CounterSystem>();
  systems.configure();

  systems.update_all(0.0);
  Position::Handle position;
  Direction::Handle direction;
  Counter::Handle counter;
  for (auto entity : created_entities) {
    entity.unpack<Position, Direction, Counter>(position, direction, counter);
    if (position && direction) {
      REQUIRE(2.0 == Approx(position->x));
      REQUIRE(3.0 == Approx(position->y));
    } else if (position) {
      REQUIRE(1.0 == Approx(position->x));
      REQUIRE(2.0 == Approx(position->y));
    }
    REQUIRE(1 == counter->counter);
  }
}
