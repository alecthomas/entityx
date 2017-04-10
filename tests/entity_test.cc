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
#include <set>
#include <map>
#include "./3rdparty/catch.hh"
#include "entityx/entityx.hh"

using std::ostream;
using std::vector;
using std::set;
using std::map;
using std::pair;
using std::string;

using entityx::EntityX;
using entityx::DefaultStorage;
using entityx::ContiguousStorage;


struct Position {
  explicit Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  bool operator==(const Position &other) const {
    return x == other.x && y == other.y;
  }

  float x, y;
};

ostream &operator<<(ostream &out, const Position &position) {
  out << "Position(" << position.x << ", " << position.y << ")";
  return out;
}

struct Direction {
  explicit Direction(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  bool operator==(const Direction &other) const {
    return x == other.x && y == other.y;
  }

  float x, y;
};

ostream &operator<<(ostream &out, const Direction &direction) {
  out << "Direction(" << direction.x << ", " << direction.y << ")";
  return out;
}

struct Tag {
  explicit Tag(string tag) : tag(tag) {}

  bool operator==(const Tag &other) const { return tag == other.tag; }

  string tag;
};

ostream &operator<<(ostream &out, const Tag &tag) {
  out << "Tag(" << tag.tag << ")";
  return out;
}


struct CopyVerifier {
  CopyVerifier() : copied(false) {}
  CopyVerifier(const CopyVerifier &other) {
    copied = other.copied + 1;
  }

  int copied;
};


struct Freed {
  explicit Freed(bool &yes) : yes(yes) {}
  ~Freed() { yes = true; }

  bool &yes;
};


struct Test {
  explicit Test(bool &yes) : freed(yes) {}

  Freed freed;
};


using EntityManager = EntityX<DefaultStorage, entityx::OBSERVABLE, Position, Direction, Tag, CopyVerifier, Test>;
using Entity = EntityManager::Entity;

template <typename T>
int size(const T &t) {
  return std::count_if(t.begin(), t.end(), [](const Entity &) { return true; });
}


struct EntityManagerFixture {
  EntityManagerFixture() {}

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
  vector<entityx::Id> entities;
  Entity e = em.create();
  Entity f = em.create();
  Entity g = em.create();
  std::vector<std::pair<Position*, Direction*>> position_directions;
  position_directions.push_back(std::make_pair(
      e.assign<Position>(1.0f, 2.0f), e.assign<Direction>(3.0f, 4.0f)));
  position_directions.push_back(std::make_pair(
      f.assign<Position>(7.0f, 8.0f), f.assign<Direction>(9.0f, 10.0f)));
  auto thetag = f.assign<Tag>("tag");
  g.assign<Position>(5.0f, 6.0f);
  int i = 0;

  REQUIRE(3 ==  size(em.entities_with_components<Position>()));

  em.for_each<Position, Direction>([&](Entity, Position &position, Direction &direction) {
    auto pd = position_directions.at(i);
    REQUIRE(position ==  *pd.first);
    REQUIRE(direction ==  *pd.second);
    ++i;
  });
  REQUIRE(2 ==  i);
  i = 0;
  em.for_each<Position, Direction, Tag>([&](Entity, Position &position, Direction &direction, Tag &tag) {
    auto pd = position_directions.at(1);
    REQUIRE(position ==  *pd.first);
    REQUIRE(direction ==  *pd.second);
    REQUIRE(tag == *thetag);
    i++;
  });
  REQUIRE(1 ==  i);
}

TEST_CASE_METHOD(EntityManagerFixture, "TestIterateAllEntitiesSkipsDestroyed") {
  Entity a = em.create();
  Entity b = em.create();
  Entity c = em.create();

  b.destroy();

  auto entities = em.all_entities();
  auto it = entities.begin();
  REQUIRE(a.id() == (*it).id());
  ++it;
  REQUIRE(c.id().index() == (*it).id().index());
}

TEST_CASE_METHOD(EntityManagerFixture, "TestUnpack") {
  Entity e = em.create();
  auto p = e.assign<Position>(1.0, 2.0);
  auto d = e.assign<Direction>(3.0, 4.0);
  auto t = e.assign<Tag>("tag");

  Position *up;
  Direction *ud;
  Tag *ut;
  em.unpack(e.id(), up);
  REQUIRE(p ==  up);
  em.unpack(e.id(), up, ud);
  REQUIRE(p ==  up);
  REQUIRE(d ==  ud);
  em.unpack(e.id(), up, ud, ut);
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

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityCreatedEvent") {
  vector<Entity> created;

  em.on_entity_created([&](Entity entity) {
    created.push_back(entity);
  });

  REQUIRE(0UL ==  created.size());
  for (int i = 0; i < 10; ++i) {
    em.create();
  }
  REQUIRE(10UL ==  created.size());
}

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityDestroyedEvent") {
  vector<Entity> destroyed;

  em.on_entity_destroyed([&](Entity entity) {
    destroyed.push_back(entity);
  });

  REQUIRE(0UL ==  destroyed.size());
  vector<Entity> entities;
  for (int i = 0; i < 10; ++i) {
    entities.push_back(em.create());
  }
  REQUIRE(0UL ==  destroyed.size());
  for (auto e : entities) {
    e.destroy();
  }
  REQUIRE(10UL == destroyed.size());
  REQUIRE(entities ==  destroyed);
}

TEST_CASE_METHOD(EntityManagerFixture, "TestComponentAddedEvent") {
  int position_events = 0;
  int direction_events = 0;

  auto on_position_added = [&position_events](Entity entity, Position *p) {
    float n = static_cast<float>(position_events);
    REQUIRE(p->x ==  n);
    REQUIRE(p->y ==  n);
    position_events++;
  };

  auto on_direction_added = [&direction_events](Entity entity, Direction *p) {
    float n = static_cast<float>(direction_events);
    REQUIRE(p->x ==  -n);
    REQUIRE(p->y ==  -n);
    direction_events++;
  };

  em.on_component_added<Position>(on_position_added);
  em.on_component_added<Direction>(on_direction_added);

  REQUIRE(0 ==  position_events);
  REQUIRE(0 ==  direction_events);
  for (int i = 0; i < 10; ++i) {
    Entity e = em.create();
    e.assign<Position>(static_cast<float>(i), static_cast<float>(i));
    e.assign<Direction>(static_cast<float>(-i), static_cast<float>(-i));
  }
  REQUIRE(10 ==  position_events);
  REQUIRE(10 ==  direction_events);
}


TEST_CASE_METHOD(EntityManagerFixture, "TestComponentRemovedEvent") {
  Direction *removed = nullptr;

  auto on_direction_removed = [&](Entity entity, Direction *event) {
    removed = event;
  };

  em.on_component_removed<Direction>(on_direction_removed);

  REQUIRE(!(removed));
  Entity e = em.create();
  Direction *p = e.assign<Direction>(1.0, 2.0);
  e.remove<Direction>();
  REQUIRE(removed ==  p);
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

  for (int i = 0; i < 5000; i++) {
    auto e = em.create();
    e.assign<Position>();
    entities.push_back(e);
  }

  REQUIRE(size(em.entities_with_components<Position>()) ==  5000);

  entities[2500].destroy();

  REQUIRE(size(em.entities_with_components<Position>()) ==  4999);
}

// TODO(alec): Disable this on OSX - it doesn't seem to be possible to catch it?!?
// TEST_CASE_METHOD(EntityManagerFixture, "DeleteComponentThrowsBadAlloc") {
//   Position *position = new Position();
//   REQUIRE_THROWS_AS(delete position, std::bad_alloc);
// }

TEST_CASE_METHOD(EntityManagerFixture, "TestComponentAssignmentFromCopy") {
  Entity a = em.create();
  CopyVerifier original;
  CopyVerifier *copy = a.assign_from_copy(original);
  REQUIRE(copy);
  REQUIRE(copy->copied == 1);
}

TEST_CASE_METHOD(EntityManagerFixture, "TestComponentInvalidatedWhenDestroyed") {
  Entity a = em.create();
  Position *position = a.assign<Position>(1, 2);
  REQUIRE(position);
  REQUIRE(position->x == 1);
  REQUIRE(position->y == 2);
  a.remove<Position>();
  REQUIRE(!a.component<Position>());
}

TEST_CASE_METHOD(EntityManagerFixture, "TestDeleteEntityWithNoComponents") {
  Entity a = em.create();
  a.assign<Position>(1, 2);
  Entity b = em.create();
  b.destroy();
}

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityInStdSet") {
  Entity a = em.create();
  Entity b = em.create();
  Entity c = em.create();
  set<Entity> entitySet;
  REQUIRE(entitySet.insert(a).second);
  REQUIRE(entitySet.insert(b).second);
  REQUIRE(entitySet.insert(c).second);
}

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityInStdMap") {
  Entity a = em.create();
  Entity b = em.create();
  Entity c = em.create();
  map<Entity, int> entityMap;
  REQUIRE(entityMap.insert(pair<Entity, int>(a, 1)).second);
  REQUIRE(entityMap.insert(pair<Entity, int>(b, 2)).second);
  REQUIRE(entityMap.insert(pair<Entity, int>(c, 3)).second);
  REQUIRE(entityMap[a] == 1);
  REQUIRE(entityMap[b] == 2);
  REQUIRE(entityMap[c] == 3);
}

TEST_CASE_METHOD(EntityManagerFixture, "TestEntityComponentsFromTuple") {
  Entity e = em.create();
  e.assign<Position>(1, 2);
  e.assign<Direction>(3, 4);

  std::tuple<Position*, Direction*> components = e.components<Position, Direction>();

  REQUIRE(std::get<0>(components)->x == 1);
  REQUIRE(std::get<0>(components)->y == 2);
  REQUIRE(std::get<1>(components)->x == 3);
  REQUIRE(std::get<1>(components)->y == 4);
}

TEST_CASE("TestComponentDestructorCalledWhenManagerDestroyed") {
  bool freed = false;
  {
    EntityManager e;
    auto test = e.create();
    test.assign<Test>(freed);
  }
  REQUIRE(freed == true);
}

TEST_CASE("TestComponentDestructorCalledWhenEntityDestroyed") {
  struct Freed {
    explicit Freed(bool &yes) : yes(yes) {}
    ~Freed() { yes = true; }

    bool &yes;
  };

  bool freed = false;
  EntityManager e;
  auto test = e.create();
  test.assign<Test>(freed);
  REQUIRE(freed == false);
  test.destroy();
  REQUIRE(freed == true);
}

TEST_CASE("TestEntityManagerDestructorCallsDestroyedEvent") {
  int destroyed = 0;
  {
    EntityManager e;
    e.on_entity_destroyed([&](Entity entity) {
      destroyed++;
    });
    e.create();
    e.create().destroy();
  }
  REQUIRE(destroyed == 2);
}

TEST_CASE("TestContiguousStorage") {
  using Storage = ContiguousStorage<10, 10, Position>;

  Storage storage;
  storage.resize(200);
  for (int i = 0; i < 200; i++)
    storage.create<Position>(i, i, -i);
  for (int i = 0; i < 200; i++) {
    Position *position = storage.get<Position>(i);
    REQUIRE(position->x == i);
    REQUIRE(position->y == -i);
  }
}

TEST_CASE("TestOnRemoveComponentCalledWhenEntityIsDestroyed") {
  int on_removed = 0;

  EntityManager em;
  em.on_component_removed<Position>([&](Entity e, Position *c) {
    on_removed++;
  });
  em.on_component_removed<Direction>([&](Entity e, Direction *c) {
    on_removed++;
  });
  Entity entity = em.create();
  entity.assign<Position>();
  entity.assign<Direction>();
  entity.destroy();

  REQUIRE(on_removed == 2);
}
