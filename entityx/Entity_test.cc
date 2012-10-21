/**
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 * 
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include <iterator>
#include <string>
#include <vector>
#include <boost/ref.hpp>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include "entityx/Entity.h"

using namespace std;
using namespace boost;
using namespace entity;


template <typename T>
int size(const T &t) {
  int n = 0;
  for (auto i : t) {
    ++n;
  }
  return n;
}

struct Position : Component<Position> {
  Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  bool operator == (const Position &other) const { return x == other.x && y == other.y; }

  float x, y;
};


ostream &operator << (ostream &out, const Position &position) {
  out << "Position(" << position.x << ", " << position.y << ")";
  return out;
}


struct Direction : Component<Direction> {
  Direction(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  bool operator == (const Direction &other) const { return x == other.x && y == other.y; }

  float x, y;
};


ostream &operator << (ostream &out, const Direction &direction) {
  out << "Direction(" << direction.x << ", " << direction.y << ")";
  return out;
}


class EntityManagerTest : public ::testing::Test {
 protected:
  EntityManagerTest() : em(ev) {}

  EventManager ev;
  EntityManager em;

  virtual void SetUp() {
  }
};


TEST_F(EntityManagerTest, TestCreateEntity) {
  Entity e = em.create();
  ASSERT_TRUE(em.exists(e));
}

TEST_F(EntityManagerTest, TestComponentConstruction) {
  auto e = em.create();
  auto p = em.assign<Position>(e, 1, 2);
  auto cp = em.component<Position>(e);
  ASSERT_EQ(p, cp);
  ASSERT_FLOAT_EQ(1.0, cp->x);
  ASSERT_FLOAT_EQ(2.0, cp->y);
}

TEST_F(EntityManagerTest, TestComponentCreationWithObject) {
  auto e = em.create();
  auto p = em.assign(e, make_shared<Position>(1.0, 2.0));
  auto cp = em.component<Position>(e);
  ASSERT_EQ(p, cp);
  ASSERT_FLOAT_EQ(1.0, cp->x);
  ASSERT_FLOAT_EQ(2.0, cp->y);
}

TEST_F(EntityManagerTest, TestDestroyEntity) {
  Entity e = em.create();
  Entity f = em.create();
  auto ep = em.assign<Position>(e);
  em.assign<Position>(f);
  em.assign<Direction>(e);
  em.assign<Direction>(f);

  ASSERT_EQ(2, ep.use_count());
  ASSERT_TRUE(em.exists(e));
  ASSERT_TRUE(em.exists(f));
  ASSERT_TRUE(em.component<Position>(e));
  ASSERT_TRUE(em.component<Direction>(e));
  ASSERT_TRUE(em.component<Position>(f));
  ASSERT_TRUE(em.component<Direction>(f));

  em.destroy(e);

  ASSERT_FALSE(em.exists(e));
  ASSERT_TRUE(em.exists(f));
  ASSERT_FALSE(em.component<Position>(e));
  ASSERT_FALSE(em.component<Direction>(e));
  ASSERT_TRUE(em.component<Position>(f));
  ASSERT_TRUE(em.component<Direction>(f));
  ASSERT_EQ(1, ep.use_count());
}

TEST_F(EntityManagerTest, TestGetEntitiesWithComponent) {
  Entity e = em.create();
  Entity f = em.create();
  Entity g = em.create();
  em.assign<Position>(e);
  em.assign<Direction>(e);
  em.assign<Position>(f);
  em.assign<Position>(g);
  ASSERT_EQ(3, size(em.entities_with_components<Position>()));
  ASSERT_EQ(1, size(em.entities_with_components<Direction>()));
}

TEST_F(EntityManagerTest, TestGetEntitiesWithIntersectionOfComponents) {
  vector<Entity> entities;
  for (int i = 0; i < 150; ++i) {
    Entity e = em.create();
    entities.push_back(e);
    if (i % 2 == 0)
      em.assign<Position>(e);
    if (i % 3 == 0)
      em.assign<Direction>(e);

  }
  ASSERT_EQ(50, size(em.entities_with_components<Direction>()));
  ASSERT_EQ(75, size(em.entities_with_components<Position>()));
  ASSERT_EQ(25, size(em.entities_with_components<Direction, Position>()));
}

TEST_F(EntityManagerTest, TestGetEntitiesWithComponentAndUnpacking) {
  vector<Entity> entities;
  Entity e = em.create();
  Entity f = em.create();
  Entity g = em.create();
  std::vector<std::pair<boost::shared_ptr<Position>, boost::shared_ptr<Direction>>> position_directions;
  position_directions.push_back(std::make_pair(
          em.assign<Position>(e, 1.0f, 2.0f),
          em.assign<Direction>(e, 3.0f, 4.0f)));
  em.assign<Position>(f, 5.0f, 6.0f);
  position_directions.push_back(std::make_pair(
          em.assign<Position>(g, 7.0f, 8.0f),
          em.assign<Direction>(g, 9.0f, 10.0f)));
  Position *position;
  Direction *direction;
  int i = 0;
  for (auto unused_entity : em.entities_with_components(position, direction)) {
    (void)unused_entity;
    ASSERT_TRUE(position != nullptr);
    ASSERT_TRUE(direction != nullptr);
    auto pd = position_directions.at(i);
    ASSERT_EQ(*position, *pd.first);
    ASSERT_EQ(*direction, *pd.second);
    ++i;
  }
  ASSERT_EQ(2, i);
}

TEST_F(EntityManagerTest, TestUnpack) {
  Entity e = em.create();
  auto p = em.assign<Position>(e);
  auto d = em.assign<Direction>(e);

  Position *up;
  Direction *ud;
  em.unpack<Position, Direction>(e, up, ud);
  ASSERT_EQ(p.get(), up);
  ASSERT_EQ(d.get(), ud);
}

TEST_F(EntityManagerTest, TestUnpackNullMissing) {
  Entity e = em.create();
  auto p = em.assign<Position>(e);

  Position *up = reinterpret_cast<Position*>(0Xdeadbeef);
  Direction *ud = reinterpret_cast<Direction*>(0Xdeadbeef);
  ASSERT_EQ(reinterpret_cast<Direction*>(0xdeadbeef), ud);
  em.unpack<Position, Direction>(e, up, ud);
  ASSERT_EQ(p.get(), up);
  ASSERT_EQ(nullptr, ud);
}

TEST_F(EntityManagerTest, TestComponentIdsDiffer) {
  ASSERT_NE(Position::family(), Direction::family());
}

TEST_F(EntityManagerTest, TestEntityCreatedEvent) {
  struct EntityCreatedEventReceiver : public Receiver<EntityCreatedEventReceiver> {
    void receive(const EntityCreatedEvent &event) {
      created.push_back(event.entity);
    }

    vector<Entity> created;
  };

  EntityCreatedEventReceiver receiver;
  ev.subscribe<EntityCreatedEvent>(receiver);

  ASSERT_EQ(0, receiver.created.size());
  for (int i = 0; i < 10; ++i) {
    em.create();
  }
  ASSERT_EQ(10, receiver.created.size());
};

TEST_F(EntityManagerTest, TestEntityDestroyedEvent) {
  struct EntityDestroyedEventReceiver : public Receiver<EntityDestroyedEventReceiver> {
    void receive(const EntityDestroyedEvent &event) {
      destroyed.push_back(event.entity);
    }

    vector<Entity> destroyed;
  };

  EntityDestroyedEventReceiver receiver;
  ev.subscribe<EntityDestroyedEvent>(receiver);

  ASSERT_EQ(0, receiver.destroyed.size());
  vector<Entity> entities;
  for (int i = 0; i < 10; ++i) {
    entities.push_back(em.create());
  }
  ASSERT_EQ(0, receiver.destroyed.size());
  for (auto e : entities) {
    em.destroy(e);
  }
  ASSERT_TRUE(entities == receiver.destroyed);
};

TEST_F(EntityManagerTest, TestComponentAddedEvent) {
  struct ComponentAddedEventReceiver : public Receiver<ComponentAddedEventReceiver> {
    void receive(const ComponentAddedEvent<Position> &event) {
      auto p = event.component;
      float n = float(created.size());
      ASSERT_EQ(p->x, n);
      ASSERT_EQ(p->y, n);
      created.push_back(event.entity);
    }

    vector<Entity> created;
  };

  ComponentAddedEventReceiver receiver;
  ev.subscribe<ComponentAddedEvent<Position>>(receiver);

  ASSERT_EQ(0, receiver.created.size());
  for (int i = 0; i < 10; ++i) {
    Entity e = em.create();
    em.assign<Position>(e, float(i), float(i));
  }
  ASSERT_EQ(10, receiver.created.size());
};
