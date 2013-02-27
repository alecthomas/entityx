/**
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include <string>
#include <vector>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include "entityx/Manager.h"
#include "entityx/System.h"


// using namespace std; // This will give name space conflicts with boost
using namespace boost;
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
  MovementSystem(string label = "") : label(label) {}

  void update(EntityManager &es, EventManager &events, double) override {
    EntityManager::View entities = es.entities_with_components<Position, Direction>();
    shared_ptr<Position> position;
    shared_ptr<Direction> direction;
    for (auto entity : entities) {
      es.unpack<Position, Direction>(entity, position, direction);
      position->x += direction->x;
      position->y += direction->y;
    }
  }

  string label;
};


class TestManager : public entityx::Manager {
 public:
  std::vector<Entity::Id> entities;

  SystemManager &sm() { return system_manager; }
  EntityManager &em() { return entity_manager; }

 protected:
  void configure() override {
  }

  void initialize() override {
    for (int i = 0; i < 150; ++i) {
      Entity::Id e = entity_manager.create();
      entities.push_back(e);
      if (i % 2 == 0)
        entity_manager.assign<Position>(e, 1, 2);
      if (i % 3 == 0)
        entity_manager.assign<Direction>(e, 1, 1);
    }
  }

  void update(double dt) override {
  }
};


class SystemManagerTest : public ::testing::Test {
 protected:
  TestManager manager;

  virtual void SetUp() override {
    manager.start();
  }
};


TEST_F(SystemManagerTest, TestConstructSystemWithArgs) {
  manager.sm().add<MovementSystem>("movement");
  manager.sm().configure();

  ASSERT_EQ("movement", manager.sm().system<MovementSystem>()->label);
}


TEST_F(SystemManagerTest, TestApplySystem) {
  manager.sm().add<MovementSystem>();
  manager.sm().configure();

  manager.sm().update<MovementSystem>(0.0);
  shared_ptr<Position> position;
  shared_ptr<Direction> direction;
  for (auto entity : manager.entities) {
    manager.em().unpack<Position, Direction>(entity, position, direction);
    if (position && direction) {
      ASSERT_FLOAT_EQ(2.0, position->x);
      ASSERT_FLOAT_EQ(3.0, position->y);
    } else if (position) {
      ASSERT_FLOAT_EQ(1.0, position->x);
      ASSERT_FLOAT_EQ(2.0, position->y);
    }
  }
}
