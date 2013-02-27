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

// using namespace std; // This will give name space conflicts with boost
using namespace boost;
using namespace entityx;

using std::ostream;
using std::vector;

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
  auto p = e.assign<Position>(1, 2);
  //auto p = em.assign<Position>(e, 1, 2);
  auto cp = e.component<Position>();
  ASSERT_EQ(p, cp);
  ASSERT_FLOAT_EQ(1.0, cp->x);
  ASSERT_FLOAT_EQ(2.0, cp->y);
}

TEST_F(EntityManagerTest, TestComponentCreationWithObject) {
  auto e = em.create();
  auto p = e.assign(make_shared<Position>(1.0, 2.0));
  auto cp = e.component<Position>();
  ASSERT_EQ(p, cp);
  ASSERT_FLOAT_EQ(1.0, cp->x);
  ASSERT_FLOAT_EQ(2.0, cp->y);
}

TEST_F(EntityManagerTest, TestDestroyEntity) {
  Entity e = em.create();
  Entity f = em.create();
  auto ep = e.assign<Position>();
  f.assign<Position>();
  e.assign<Direction>(e);
  f.assign<Direction>();

  ASSERT_EQ(2, ep.use_count());
  ASSERT_TRUE(em.exists(e));
  ASSERT_TRUE(em.exists(f));
  ASSERT_TRUE(e.component<Position>());
  ASSERT_TRUE(e.component<Direction>());
  ASSERT_TRUE(f.component<Position>());
  ASSERT_TRUE(f.component<Direction>());

  em.destroy(e);

  ASSERT_FALSE(em.exists(e));
  ASSERT_TRUE(em.exists(f));
  ASSERT_FALSE(e.component<Position>());
  ASSERT_FALSE(e.component<Direction>());
  ASSERT_TRUE(f.component<Position>());
  ASSERT_TRUE(f.component<Direction>());
  ASSERT_EQ(1, ep.use_count());
}

TEST_F(EntityManagerTest, TestGetEntitiesWithComponent) {
  Entity e = em.create();
  Entity f = em.create();
  Entity g = em.create();
  e.assign<Position>();
  e.assign<Direction>();
  f.assign<Position>();
  g.assign<Position>();
  ASSERT_EQ(3, size(em.entities_with_components<Position>()));
  ASSERT_EQ(1, size(em.entities_with_components<Direction>()));
}

TEST_F(EntityManagerTest, TestGetEntitiesWithIntersectionOfComponents) {
  vector<Entity::Id> entities;
  for (int i = 0; i < 150; ++i) {
    Entity e = em.create();
    entities.push_back(e);
    if (i % 2 == 0)
      e.assign<Position>();
    if (i % 3 == 0)
      e.assign<Direction>();

  }
  ASSERT_EQ(50, size(em.entities_with_components<Direction>()));
  ASSERT_EQ(75, size(em.entities_with_components<Position>()));
  ASSERT_EQ(25, size(em.entities_with_components<Direction, Position>()));
}

TEST_F(EntityManagerTest, TestGetEntitiesWithComponentAndUnpacking) {
  vector<Entity::Id> entities;
  Entity::Id e = em.create();
  Entity::Id f = em.create();
  Entity::Id g = em.create();
  std::vector<std::pair<shared_ptr<Position>, shared_ptr<Direction>>> position_directions;
  position_directions.push_back(std::make_pair(
          em.assign<Position>(e, 1.0f, 2.0f),
          em.assign<Direction>(e, 3.0f, 4.0f)));
  position_directions.push_back(std::make_pair(
          em.assign<Position>(f, 7.0f, 8.0f),
          em.assign<Direction>(f, 9.0f, 10.0f)));
  em.assign<Position>(g, 5.0f, 6.0f);
  int i = 0;

  shared_ptr<Position> position;
  shared_ptr<Direction> direction;
  for (auto unused_entity : em.entities_with_components(position, direction)) {
    (void)unused_entity;
    ASSERT_TRUE(position);
    ASSERT_TRUE(direction);
    auto pd = position_directions.at(i);
    ASSERT_EQ(position, pd.first);
    ASSERT_EQ(direction, pd.second);
    ++i;
  }
  ASSERT_EQ(2, i);
}

TEST_F(EntityManagerTest, TestUnpack) {
  Entity::Id e = em.create();
  auto p = em.assign<Position>(e);
  auto d = em.assign<Direction>(e);

  shared_ptr<Position> up;
  shared_ptr<Direction> ud;
  em.unpack<Position, Direction>(e, up, ud);
  ASSERT_EQ(p, up);
  ASSERT_EQ(d, ud);
}

// gcc 4.7.2 does not allow this struct to be declared locally inside the TEST_F.
struct NullDeleter {template<typename T> void operator()(T*) {} };

TEST_F(EntityManagerTest, TestUnpackNullMissing) {
  Entity::Id e = em.create();
  auto p = em.assign<Position>(e);

  shared_ptr<Position> up(reinterpret_cast<Position*>(0Xdeadbeef), NullDeleter());
  shared_ptr<Direction> ud(reinterpret_cast<Direction*>(0Xdeadbeef), NullDeleter());
  em.unpack<Position, Direction>(e, up, ud);
  ASSERT_EQ(p, up);
  ASSERT_EQ(shared_ptr<Direction>(), ud);
}

TEST_F(EntityManagerTest, TestComponentIdsDiffer) {
  ASSERT_NE(Position::family(), Direction::family());
}

TEST_F(EntityManagerTest, TestEntityCreatedEvent) {
  struct EntityCreatedEventReceiver : public Receiver<EntityCreatedEventReceiver> {
    void receive(const EntityCreatedEvent &event) {
      created.push_back(event.entity);
    }

    vector<Entity::Id> created;
  };

  EntityCreatedEventReceiver receiver;
  ev.subscribe<EntityCreatedEvent>(receiver);

  ASSERT_EQ(0, receiver.created.size());
  for (int i = 0; i < 10; ++i) {
    em.create();
  }
  ASSERT_EQ(10, receiver.created.size());
}

TEST_F(EntityManagerTest, TestEntityDestroyedEvent) {
  struct EntityDestroyedEventReceiver : public Receiver<EntityDestroyedEventReceiver> {
    void receive(const EntityDestroyedEvent &event) {
      destroyed.push_back(event.entity);
    }

    vector<Entity::Id> destroyed;
  };

  EntityDestroyedEventReceiver receiver;
  ev.subscribe<EntityDestroyedEvent>(receiver);

  ASSERT_EQ(0, receiver.destroyed.size());
  vector<Entity::Id> entities;
  for (int i = 0; i < 10; ++i) {
    entities.push_back(em.create());
  }
  ASSERT_EQ(0, receiver.destroyed.size());
  for (auto e : entities) {
    em.destroy(e);
  }
  ASSERT_TRUE(entities == receiver.destroyed);
}

TEST_F(EntityManagerTest, TestComponentAddedEvent) {
  struct ComponentAddedEventReceiver : public Receiver<ComponentAddedEventReceiver> {
    void receive(const ComponentAddedEvent<Position> &event) {
      auto p = event.component;
      float n = float(position_events);
      ASSERT_EQ(p->x, n);
      ASSERT_EQ(p->y, n);
      position_events++;
    }

    void receive(const ComponentAddedEvent<Direction> &event) {
      auto p = event.component;
      float n = float(direction_events);
      ASSERT_EQ(p->x, -n);
      ASSERT_EQ(p->y, -n);
      direction_events++;
    }

    int position_events = 0;
    int direction_events = 0;
  };

  ComponentAddedEventReceiver receiver;
  ev.subscribe<ComponentAddedEvent<Position>>(receiver);
  ev.subscribe<ComponentAddedEvent<Direction>>(receiver);

  ASSERT_NE(ComponentAddedEvent<Position>::family(),
            ComponentAddedEvent<Direction>::family());

  ASSERT_EQ(0, receiver.position_events);
  ASSERT_EQ(0, receiver.direction_events);
  for (int i = 0; i < 10; ++i) {
    Entity::Id e = em.create();
    em.assign<Position>(e, float(i), float(i));
    em.assign<Direction>(e, float(-i), float(-i));
  }
  ASSERT_EQ(10, receiver.position_events);
  ASSERT_EQ(10, receiver.direction_events);
}
