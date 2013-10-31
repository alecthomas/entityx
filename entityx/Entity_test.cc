/*
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "entityx/Entity.h"

// using namespace std;
using namespace entityx;

using std::ostream;
using std::vector;

template <typename T>
int size(const T &t) {
  int n = 0;
  for (auto i : t) {
    ++n;
    (void)i;  // Unused on purpose, suppress warning
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
  EntityManagerTest() : ev(EventManager::make()), em(EntityManager::make(ev)) {}

  ptr<EventManager> ev;
  ptr<EntityManager> em;

  virtual void SetUp() {
  }
};


TEST_F(EntityManagerTest, TestCreateEntity) {
  ASSERT_EQ(em->size(), 0UL);

  Entity e2;
  ASSERT_FALSE(e2.valid());

  Entity e = em->create();
  ASSERT_TRUE(e.valid());
  ASSERT_EQ(em->size(), 1UL);

  e2 = e;
  ASSERT_TRUE(e2.valid());
}

TEST_F(EntityManagerTest, TestEntityAsBoolean) {
  ASSERT_EQ(em->size(), 0UL);
  Entity e = em->create();
  ASSERT_TRUE(e.valid());
  ASSERT_EQ(em->size(), 1UL);
  ASSERT_FALSE(!e);

  e.destroy();

  ASSERT_EQ(em->size(), 0UL);

  ASSERT_TRUE(!e);

  Entity e2;  // Not initialized
  ASSERT_TRUE(!e2);
}

TEST_F(EntityManagerTest, TestEntityReuse) {
  Entity e1 = em->create();
  Entity e2 = e1;
  auto id = e1.id();
  ASSERT_TRUE(e1.valid());
  ASSERT_TRUE(e2.valid());
  e1.destroy();
  ASSERT_TRUE(!e1.valid());
  ASSERT_TRUE(!e2.valid());
  Entity e3 = em->create();
  // It is assumed that the allocation will reuse the same entity id, though
  // the version will change.
  auto new_id = e3.id();
  ASSERT_NE(new_id, id);
  ASSERT_EQ(new_id.id() & 0xffffffffUL, id.id() & 0xffffffffUL);
}

TEST_F(EntityManagerTest, TestComponentConstruction) {
  auto e = em->create();
  auto p = e.assign<Position>(1, 2);
  auto cp = e.component<Position>();
  ASSERT_EQ(p, cp);
  ASSERT_FLOAT_EQ(1.0, cp->x);
  ASSERT_FLOAT_EQ(2.0, cp->y);
}

TEST_F(EntityManagerTest, TestComponentCreationWithObject) {
  auto e = em->create();
  auto p = e.assign(ptr<Position>(new Position(1.0, 2.0)));
  auto cp = e.component<Position>();
  ASSERT_EQ(p, cp);
  ASSERT_FLOAT_EQ(1.0, cp->x);
  ASSERT_FLOAT_EQ(2.0, cp->y);
}

TEST_F(EntityManagerTest, TestDestroyEntity) {
  Entity e = em->create();
  Entity f = em->create();
  auto ep = e.assign<Position>();
  f.assign<Position>();
  e.assign<Direction>();
  f.assign<Direction>();

  ASSERT_EQ(2, use_count(ep));
  ASSERT_TRUE(e.valid());
  ASSERT_TRUE(f.valid());
  ASSERT_TRUE(static_cast<bool>(e.component<Position>()));
  ASSERT_TRUE(static_cast<bool>(e.component<Direction>()));
  ASSERT_TRUE(static_cast<bool>(f.component<Position>()));
  ASSERT_TRUE(static_cast<bool>(f.component<Direction>()));

  e.destroy();

  ASSERT_FALSE(e.valid());
  ASSERT_TRUE(f.valid());
  ASSERT_TRUE(static_cast<bool>(f.component<Position>()));
  ASSERT_TRUE(static_cast<bool>(f.component<Direction>()));
  ASSERT_EQ(1, use_count(ep));
}

TEST_F(EntityManagerTest, TestGetEntitiesWithComponent) {
  Entity e = em->create();
  Entity f = em->create();
  Entity g = em->create();
  e.assign<Position>();
  e.assign<Direction>();
  f.assign<Position>();
  g.assign<Position>();
  ASSERT_EQ(3, size(em->entities_with_components<Position>()));
  ASSERT_EQ(1, size(em->entities_with_components<Direction>()));
}

TEST_F(EntityManagerTest, TestGetEntitiesWithIntersectionOfComponents) {
  vector<Entity> entities;
  for (int i = 0; i < 150; ++i) {
    Entity e = em->create();
    entities.push_back(e);
    if (i % 2 == 0)
      e.assign<Position>();
    if (i % 3 == 0)
      e.assign<Direction>();
  }
  ASSERT_EQ(50, size(em->entities_with_components<Direction>()));
  ASSERT_EQ(75, size(em->entities_with_components<Position>()));
  ASSERT_EQ(25, size(em->entities_with_components<Direction, Position>()));
}

TEST_F(EntityManagerTest, TestGetEntitiesWithComponentAndUnpacking) {
  vector<Entity::Id> entities;
  Entity e = em->create();
  Entity f = em->create();
  Entity g = em->create();
  std::vector<std::pair<ptr<Position>, ptr<Direction>>> position_directions;
  position_directions.push_back(std::make_pair(
          e.assign<Position>(1.0f, 2.0f),
          e.assign<Direction>(3.0f, 4.0f)));
  position_directions.push_back(std::make_pair(
          f.assign<Position>(7.0f, 8.0f),
          f.assign<Direction>(9.0f, 10.0f)));
  g.assign<Position>(5.0f, 6.0f);
  int i = 0;

  ptr<Position> position;
  ptr<Direction> direction;
  for (auto unused_entity : em->entities_with_components(position, direction)) {
    (void)unused_entity;
    ASSERT_TRUE(static_cast<bool>(position));
    ASSERT_TRUE(static_cast<bool>(direction));
    auto pd = position_directions.at(i);
    ASSERT_EQ(position, pd.first);
    ASSERT_EQ(direction, pd.second);
    ++i;
  }
  ASSERT_EQ(2, i);
}

TEST_F(EntityManagerTest, TestUnpack) {
  Entity e = em->create();
  auto p = e.assign<Position>();
  auto d = e.assign<Direction>();

  ptr<Position> up;
  ptr<Direction> ud;
  e.unpack<Position, Direction>(up, ud);
  ASSERT_EQ(p, up);
  ASSERT_EQ(d, ud);
}

// gcc 4.7.2 does not allow this struct to be declared locally inside the TEST_F.

// TEST_F(EntityManagerTest, TestUnpackNullMissing) {
//   Entity e = em->create();
//   auto p = e.assign<Position>();

//   ptr<Position> up(reinterpret_cast<Position*>(0Xdeadbeef), NullDeleter());
//   ptr<Direction> ud(reinterpret_cast<Direction*>(0Xdeadbeef), NullDeleter());
//   e.unpack<Position, Direction>(up, ud);
//   ASSERT_EQ(p, up);
//   ASSERT_EQ(ptr<Direction>(), ud);
// }

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
  ev->subscribe<EntityCreatedEvent>(receiver);

  ASSERT_EQ(0UL, receiver.created.size());
  for (int i = 0; i < 10; ++i) {
    em->create();
  }
  ASSERT_EQ(10UL, receiver.created.size());
}

TEST_F(EntityManagerTest, TestEntityDestroyedEvent) {
  struct EntityDestroyedEventReceiver : public Receiver<EntityDestroyedEventReceiver> {
    void receive(const EntityDestroyedEvent &event) {
      destroyed.push_back(event.entity);
    }

    vector<Entity> destroyed;
  };

  EntityDestroyedEventReceiver receiver;
  ev->subscribe<EntityDestroyedEvent>(receiver);

  ASSERT_EQ(0UL, receiver.destroyed.size());
  vector<Entity> entities;
  for (int i = 0; i < 10; ++i) {
    entities.push_back(em->create());
  }
  ASSERT_EQ(0UL, receiver.destroyed.size());
  for (auto e : entities) {
    e.destroy();
  }
  ASSERT_EQ(entities, receiver.destroyed);
}

TEST_F(EntityManagerTest, TestComponentAddedEvent) {
  struct ComponentAddedEventReceiver : public Receiver<ComponentAddedEventReceiver> {
    void receive(const ComponentAddedEvent<Position> &event) {
      auto p = event.component;
      float n = static_cast<float>(position_events);
      ASSERT_EQ(p->x, n);
      ASSERT_EQ(p->y, n);
      position_events++;
    }

    void receive(const ComponentAddedEvent<Direction> &event) {
      auto p = event.component;
      float n = static_cast<float>(direction_events);
      ASSERT_EQ(p->x, -n);
      ASSERT_EQ(p->y, -n);
      direction_events++;
    }

    int position_events = 0;
    int direction_events = 0;
  };

  ComponentAddedEventReceiver receiver;
  ev->subscribe<ComponentAddedEvent<Position>>(receiver);
  ev->subscribe<ComponentAddedEvent<Direction>>(receiver);

  ASSERT_NE(ComponentAddedEvent<Position>::family(),
            ComponentAddedEvent<Direction>::family());

  ASSERT_EQ(0, receiver.position_events);
  ASSERT_EQ(0, receiver.direction_events);
  for (int i = 0; i < 10; ++i) {
    Entity e = em->create();
    e.assign<Position>(static_cast<float>(i), static_cast<float>(i));
    e.assign<Direction>(static_cast<float>(-i), static_cast<float>(-i));
  }
  ASSERT_EQ(10, receiver.position_events);
  ASSERT_EQ(10, receiver.direction_events);
}

TEST_F(EntityManagerTest, TestComponentRemovedEvent) {
  struct ComponentRemovedReceiver : public Receiver<ComponentRemovedReceiver> {
    void receive(const ComponentRemovedEvent<Direction> &event) {
      removed = event.component;
    }

    ptr<Direction> removed;
  };

  ComponentRemovedReceiver receiver;
  ev->subscribe<ComponentRemovedEvent<Direction>>(receiver);

  ASSERT_FALSE(receiver.removed);
  Entity e = em->create();
  e.assign<Direction>(1.0, 2.0);
  auto p = e.remove<Direction>();
  ASSERT_EQ(receiver.removed, p);
  ASSERT_FALSE(e.component<Direction>());
}

TEST_F(EntityManagerTest, TestEntityAssignment) {
  Entity a, b;
  a = em->create();
  ASSERT_NE(a, b);
  b = a;
  ASSERT_EQ(a, b);
  a.invalidate();
  ASSERT_NE(a, b);
}

TEST_F(EntityManagerTest, TestEntityDestroyAll) {
  Entity a = em->create(), b = em->create();
  em->destroy_all();
  ASSERT_FALSE(a.valid());
  ASSERT_FALSE(b.valid());
}


TEST_F(EntityManagerTest, TestEntityDestroyHole) {
  std::vector<Entity> entities;

  auto count = [this]() -> int {
    auto e = em->entities_with_components<Position>();
    return std::count_if(e.begin(), e.end(), [] (const Entity &) { return true; });
  };

  for (int i = 0; i < 5000; i++) {
    auto e = em->create();
    e.assign<Position>();
    entities.push_back(e);
  }

  ASSERT_EQ(count(), 5000);

  entities[2500].destroy();

  ASSERT_EQ(count(), 4999);
}
