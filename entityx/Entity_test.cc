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

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include "entityx/3rdparty/catch.hpp"
#include "entityx/Entity.h"

// using namespace std;
using namespace entityx;

using std::ostream;
using std::vector;
using std::string;

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

  bool operator==(const Position &other) const {
    return x == other.x && y == other.y;
  }

  float x, y;
};

ostream &operator<<(ostream &out, const Position &position) {
  out << "Position(" << position.x << ", " << position.y << ")";
  return out;
}

struct Direction : Component<Direction> {
  Direction(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  bool operator==(const Direction &other) const {
    return x == other.x && y == other.y;
  }

  float x, y;
};

ostream &operator<<(ostream &out, const Direction &direction) {
  out << "Direction(" << direction.x << ", " << direction.y << ")";
  return out;
}

struct Tag : Component<Tag> {
  explicit Tag(string tag) : tag(tag) {}

  bool operator==(const Tag &other) const { return tag == other.tag; }

  string tag;
};

ostream &operator<<(ostream &out, const Tag &tag) {
  out << "Tag(" << tag.tag << ")";
  return out;
}


struct EntityManagerFixture {
  EntityManagerFixture() : em(ev) {}

  EventManager ev;
  EntityManager em;
};


TEST_CASE_METHOD(EntityManagerFixture, "TestCreateEntity") {
  REQUIRE(em.size() ==  0UL);

  Entity e2;
  REQUIRE(!(e2.valid()));

  Entity e = em.create();
  REQUIRE(e.valid());
  REQUIRE(em.size() ==  1UL);

  e2 = e;
  REQUIRE(e2.valid());
}

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityAsBoolean") {
  REQUIRE(em.size() ==  0UL);
  Entity e = em.create();
  REQUIRE(e.valid());
  REQUIRE(em.size() ==  1UL);
  REQUIRE(!(!e));

  e.destroy();

  REQUIRE(em.size() ==  0UL);

  REQUIRE(!e);

  Entity e2;  // Not initialized
  REQUIRE(!e2);
}

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityReuse") {
  Entity e1 = em.create();
  Entity e2 = e1;
  auto id = e1.id();
  REQUIRE(e1.valid());
  REQUIRE(e2.valid());
  e1.destroy();
  REQUIRE(!e1.valid());
  REQUIRE(!e2.valid());
  Entity e3 = em.create();
  // It is assumed that the allocation will reuse the same entity id, though
  // the version will change.
  auto new_id = e3.id();
  REQUIRE(new_id !=  id);
  REQUIRE((new_id.id() & 0xffffffffUL) == (id.id() & 0xffffffffUL));
}

TEST_CASE_METHOD(EntityManagerFixture, "TestComponentConstruction") {
  auto e = em.create();
  auto p = e.assign<Position>(1, 2);
  auto cp = e.component<Position>();
  REQUIRE(p ==  cp);
  REQUIRE(1.0 == Approx(cp->x));
  REQUIRE(2.0 == Approx(cp->y));
}

TEST_CASE_METHOD(EntityManagerFixture, "TestDestroyEntity") {
  Entity e = em.create();
  Entity f = em.create();
  e.assign<Position>();
  f.assign<Position>();
  e.assign<Direction>();
  f.assign<Direction>();

  REQUIRE(e.valid());
  REQUIRE(f.valid());
  REQUIRE(static_cast<bool>(e.component<Position>()));
  REQUIRE(static_cast<bool>(e.component<Direction>()));
  REQUIRE(static_cast<bool>(f.component<Position>()));
  REQUIRE(static_cast<bool>(f.component<Direction>()));

  e.destroy();

  REQUIRE(!(e.valid()));
  REQUIRE(f.valid());
  REQUIRE(static_cast<bool>(f.component<Position>()));
  REQUIRE(static_cast<bool>(f.component<Direction>()));
}

TEST_CASE_METHOD(EntityManagerFixture, "TestGetEntitiesWithComponent") {
  Entity e = em.create();
  Entity f = em.create();
  Entity g = em.create();
  e.assign<Position>();
  e.assign<Direction>();
  f.assign<Position>();
  g.assign<Position>();
  REQUIRE(3 ==  size(em.entities_with_components<Position>()));
  REQUIRE(1 ==  size(em.entities_with_components<Direction>()));
}

TEST_CASE_METHOD(EntityManagerFixture, "TestGetEntitiesWithIntersectionOfComponents") {
  vector<Entity> entities;
  for (int i = 0; i < 150; ++i) {
    Entity e = em.create();
    entities.push_back(e);
    if (i % 2 == 0) e.assign<Position>();
    if (i % 3 == 0) e.assign<Direction>();
  }
  REQUIRE(50 ==  size(em.entities_with_components<Direction>()));
  REQUIRE(75 ==  size(em.entities_with_components<Position>()));
  REQUIRE(25 ==  size(em.entities_with_components<Direction, Position>()));
}

TEST_CASE_METHOD(EntityManagerFixture, "TestGetEntitiesWithComponentAndUnpacking") {
  vector<Entity::Id> entities;
  Entity e = em.create();
  Entity f = em.create();
  Entity g = em.create();
  std::vector<std::pair<Position *, Direction *>> position_directions;
  position_directions.push_back(std::make_pair(
      e.assign<Position>(1.0f, 2.0f), e.assign<Direction>(3.0f, 4.0f)));
  position_directions.push_back(std::make_pair(
      f.assign<Position>(7.0f, 8.0f), f.assign<Direction>(9.0f, 10.0f)));
  auto thetag = f.assign<Tag>("tag");
  g.assign<Position>(5.0f, 6.0f);
  int i = 0;

  Position *position;
  REQUIRE(3 ==  size(em.entities_with_components(position)));

  Direction *direction;
  for (auto unused_entity : em.entities_with_components(position, direction)) {
    (void)unused_entity;
    REQUIRE(static_cast<bool>(position));
    REQUIRE(static_cast<bool>(direction));
    auto pd = position_directions.at(i);
    REQUIRE(position ==  pd.first);
    REQUIRE(direction ==  pd.second);
    ++i;
  }
  REQUIRE(2 ==  i);
  Tag *tag;
  i = 0;
  for (auto unused_entity :
       em.entities_with_components(position, direction, tag)) {
    (void)unused_entity;
    REQUIRE(static_cast<bool>(position));
    REQUIRE(static_cast<bool>(direction));
    REQUIRE(static_cast<bool>(tag));
    auto pd = position_directions.at(1);
    REQUIRE(position ==  pd.first);
    REQUIRE(direction ==  pd.second);
    REQUIRE(tag ==  thetag);
    i++;
  }
  REQUIRE(1 ==  i);
}

TEST_CASE_METHOD(EntityManagerFixture, "TestUnpack") {
  Entity e = em.create();
  auto p = e.assign<Position>(1.0, 2.0);
  auto d = e.assign<Direction>(3.0, 4.0);
  auto t = e.assign<Tag>("tag");

  Position *up;
  Direction *ud;
  Tag *ut;
  e.unpack(up);
  REQUIRE(p ==  up);
  e.unpack(up, ud);
  REQUIRE(p ==  up);
  REQUIRE(d ==  ud);
  e.unpack(up, ud, ut);
  REQUIRE(p ==  up);
  REQUIRE(d ==  ud);
  REQUIRE(t ==  ut);
}

// gcc 4.7.2 does not allow this struct to be declared locally inside the
// TEST_CASE_METHOD.EntityManagerFixture, " //" TEST_CASE_METHOD(EntityManagerFixture, "TestUnpackNullMissing") {
//   Entity e = em.create();
//   auto p = e.assign<Position>();

//   std::shared_ptr<Position> up(reinterpret_cast<Position*>(0Xdeadbeef),
// NullDeleter());
//   std::shared_ptr<Direction> ud(reinterpret_cast<Direction*>(0Xdeadbeef),
// NullDeleter());
//   e.unpack<Position, Direction>(up, ud);
//   REQUIRE(p ==  up);
//   REQUIRE(std::shared_ptr<Direction>() ==  ud);
// }

TEST_CASE_METHOD(EntityManagerFixture, "TestComponentIdsDiffer") {
  REQUIRE(Position::family() !=  Direction::family());
}

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityCreatedEvent") {
  struct EntityCreatedEventReceiver
      : public Receiver<EntityCreatedEventReceiver> {
    void receive(const EntityCreatedEvent &event) {
      created.push_back(event.entity);
    }

    vector<Entity> created;
  };

  EntityCreatedEventReceiver receiver;
  ev.subscribe<EntityCreatedEvent>(receiver);

  REQUIRE(0UL ==  receiver.created.size());
  for (int i = 0; i < 10; ++i) {
    em.create();
  }
  REQUIRE(10UL ==  receiver.created.size());
}

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityDestroyedEvent") {
  struct EntityDestroyedEventReceiver
      : public Receiver<EntityDestroyedEventReceiver> {
    void receive(const EntityDestroyedEvent &event) {
      destroyed.push_back(event.entity);
    }

    vector<Entity> destroyed;
  };

  EntityDestroyedEventReceiver receiver;
  ev.subscribe<EntityDestroyedEvent>(receiver);

  REQUIRE(0UL ==  receiver.destroyed.size());
  vector<Entity> entities;
  for (int i = 0; i < 10; ++i) {
    entities.push_back(em.create());
  }
  REQUIRE(0UL ==  receiver.destroyed.size());
  for (auto e : entities) {
    e.destroy();
  }
  REQUIRE(entities ==  receiver.destroyed);
}

TEST_CASE_METHOD(EntityManagerFixture, "TestComponentAddedEvent") {
  struct ComponentAddedEventReceiver
      : public Receiver<ComponentAddedEventReceiver> {
    void receive(const ComponentAddedEvent<Position> &event) {
      auto p = event.component;
      float n = static_cast<float>(position_events);
      REQUIRE(p->x ==  n);
      REQUIRE(p->y ==  n);
      position_events++;
    }

    void receive(const ComponentAddedEvent<Direction> &event) {
      auto p = event.component;
      float n = static_cast<float>(direction_events);
      REQUIRE(p->x ==  -n);
      REQUIRE(p->y ==  -n);
      direction_events++;
    }

    int position_events = 0;
    int direction_events = 0;
  };

  ComponentAddedEventReceiver receiver;
  ev.subscribe<ComponentAddedEvent<Position>>(receiver);
  ev.subscribe<ComponentAddedEvent<Direction>>(receiver);

  REQUIRE(ComponentAddedEvent<Position>::family() !=
            ComponentAddedEvent<Direction>::family());

  REQUIRE(0 ==  receiver.position_events);
  REQUIRE(0 ==  receiver.direction_events);
  for (int i = 0; i < 10; ++i) {
    Entity e = em.create();
    e.assign<Position>(static_cast<float>(i), static_cast<float>(i));
    e.assign<Direction>(static_cast<float>(-i), static_cast<float>(-i));
  }
  REQUIRE(10 ==  receiver.position_events);
  REQUIRE(10 ==  receiver.direction_events);
}


TEST_CASE_METHOD(EntityManagerFixture, "TestComponentRemovedEvent") {
  struct ComponentRemovedReceiver : public Receiver<ComponentRemovedReceiver> {
    void receive(const ComponentRemovedEvent<Direction> &event) {
      removed = event.component;
    }

    Direction *removed = nullptr;
  };

  ComponentRemovedReceiver receiver;
  ev.subscribe<ComponentRemovedEvent<Direction>>(receiver);

  REQUIRE(!(receiver.removed));
  Entity e = em.create();
  Direction *p = e.assign<Direction>(1.0, 2.0);
  e.remove<Direction>();
  REQUIRE(receiver.removed ==  p);
  REQUIRE(!(e.component<Direction>()));
}

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityAssignment") {
  Entity a, b;
  a = em.create();
  REQUIRE(a !=  b);
  b = a;
  REQUIRE(a ==  b);
  a.invalidate();
  REQUIRE(a !=  b);
}

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityDestroyAll") {
  Entity a = em.create(), b = em.create();
  em.reset();
  REQUIRE(!(a.valid()));
  REQUIRE(!(b.valid()));
}

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityDestroyHole") {
  std::vector<Entity> entities;

  auto count = [this]()->int {
    auto e = em.entities_with_components<Position>();
    return std::count_if(e.begin(), e.end(),
                         [](const Entity &) { return true; });
  };

  for (int i = 0; i < 5000; i++) {
    auto e = em.create();
    e.assign<Position>();
    entities.push_back(e);
  }

  REQUIRE(count() ==  5000);

  entities[2500].destroy();

  REQUIRE(count() ==  4999);
}

TEST_CASE_METHOD(EntityManagerFixture, "DeleteComponentThrowsBadAlloc") {
  Position *position = new Position();
  REQUIRE_THROWS_AS(delete position, std::bad_alloc);
}
