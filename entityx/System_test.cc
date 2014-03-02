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
#include <string>
#include <vector>
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

class SystemManagerTest : public ::testing::Test {
 protected:
  TestContainer manager;

  virtual void SetUp() override { manager.initialize(); }
};

TEST_F(SystemManagerTest, TestConstructSystemWithArgs) {
  manager.systems.add<MovementSystem>("movement");
  manager.systems.configure();

  ASSERT_EQ("movement", manager.systems.system<MovementSystem>()->label);
}

TEST_F(SystemManagerTest, TestApplySystem) {
  manager.systems.add<MovementSystem>();
  manager.systems.configure();

  manager.systems.update<MovementSystem>(0.0);
  Position *position;
  Direction *direction;
  for (auto entity : manager.created_entities) {
    entity.unpack<Position, Direction>(position, direction);
    if (position && direction) {
      ASSERT_FLOAT_EQ(2.0, position->x);
      ASSERT_FLOAT_EQ(3.0, position->y);
    } else if (position) {
      ASSERT_FLOAT_EQ(1.0, position->x);
      ASSERT_FLOAT_EQ(2.0, position->y);
    }
  }
}
