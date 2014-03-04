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
  Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  float x, y;
};

struct Direction : Component<Direction> {
  Direction(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  float x, y;
};

class MovementSystem : public System<MovementSystem> {
 public:
  explicit MovementSystem(string label = "") : label(label) {}

  void update(EntityManager &es, EventManager &events, double) override {
    EntityManager::View entities =
        es.entities_with_components<Position, Direction>();
    Position *position;
    Direction *direction;
    for (auto entity : entities) {
      entity.unpack<Position, Direction>(position, direction);
      position->x += direction->x;
      position->y += direction->y;
    }
  }

  string label;
};

class TestContainer : public EntityX {
 public:
  std::vector<Entity> created_entities;

  void initialize() {
    for (int i = 0; i < 150; ++i) {
      Entity e = entities.create();
      created_entities.push_back(e);
      if (i % 2 == 0) e.assign<Position>(1, 2);
      if (i % 3 == 0) e.assign<Direction>(1, 1);
    }
  }
};

TEST_CASE("SystemManagerTest", "[systemmanager]") {
  TestContainer manager;
  manager.initialize();

  SECTION("TestConstructSystemWithArgs") {
    manager.systems.add<MovementSystem>("movement");
    manager.systems.configure();

    REQUIRE("movement" == manager.systems.system<MovementSystem>()->label);
  }

  SECTION("TestApplySystem") {
    manager.systems.add<MovementSystem>();
    manager.systems.configure();

    manager.systems.update<MovementSystem>(0.0);
    Position *position;
    Direction *direction;
    for (auto entity : manager.created_entities) {
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
}
