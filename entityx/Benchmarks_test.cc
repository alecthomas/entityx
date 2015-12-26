#define CATCH_CONFIG_MAIN

#include <iostream>
#include <vector>
#include "entityx/3rdparty/catch.hpp"
#include "entityx/help/Timer.h"
#include "entityx/Entity.h"

using std::uint64_t;
using std::cout;
using std::endl;
using std::vector;

using entityx::Receiver;
using entityx::Component;
using entityx::ComponentHandle;
using entityx::Entity;
using entityx::EntityCreatedEvent;
using entityx::EntityDestroyedEvent;
using entityx::EventManager;
using entityx::EntityManager;

struct AutoTimer {
  ~AutoTimer() {
    cout << timer_.elapsed() << " seconds elapsed" << endl;
  }

private:
  entityx::help::Timer timer_;
};

struct Listener : public Receiver<Listener> {
  void receive(const EntityCreatedEvent &event) { ++created; }
  void receive(const EntityDestroyedEvent &event) { ++destroyed; }

  int created = 0;
  int destroyed = 0;
};

struct Position : public Component<Position> {
};


struct Direction : public Component<Direction> {
};


struct BenchmarkFixture {
  BenchmarkFixture() : em(ev) {}

  EventManager ev;
  EntityManager em;
};


TEST_CASE_METHOD(BenchmarkFixture, "TestCreateEntities") {
  AutoTimer t;

  uint64_t count = 10000000L;
  cout << "creating " << count << " entities" << endl;

  for (uint64_t i = 0; i < count; i++) {
    em.create();
  }
}


TEST_CASE_METHOD(BenchmarkFixture, "TestDestroyEntities") {
  uint64_t count = 10000000L;
  vector<Entity> entities;
  for (uint64_t i = 0; i < count; i++) {
    entities.push_back(em.create());
  }

  AutoTimer t;
  cout << "destroying " << count << " entities" << endl;

  for (auto e : entities) {
    e.destroy();
  }
}

TEST_CASE_METHOD(BenchmarkFixture, "TestCreateEntitiesWithListener") {
  Listener listen;
  ev.subscribe<EntityCreatedEvent>(listen);

  int count = 10000000L;

  AutoTimer t;
  cout << "creating " << count << " entities while notifying a single EntityCreatedEvent listener" << endl;

  vector<Entity> entities;
  for (int i = 0; i < count; i++) {
    entities.push_back(em.create());
  }

  REQUIRE(entities.size() == count);
  REQUIRE(listen.created == count);
}

TEST_CASE_METHOD(BenchmarkFixture, "TestDestroyEntitiesWithListener") {
  int count = 10000000;
  vector<Entity> entities;
  for (int i = 0; i < count; i++) {
    entities.push_back(em.create());
  }

  Listener listen;
  ev.subscribe<EntityDestroyedEvent>(listen);

  AutoTimer t;
  cout << "destroying " << count << " entities while notifying a single EntityDestroyedEvent listener" << endl;

  for (auto &e : entities) {
    e.destroy();
  }

  REQUIRE(entities.size() == count);
  REQUIRE(listen.destroyed == count);
}

TEST_CASE_METHOD(BenchmarkFixture, "TestEntityIteration") {
  int count = 10000000;
  for (int i = 0; i < count; i++) {
    auto e = em.create();
    e.assign<Position>();
  }

  AutoTimer t;
  cout << "iterating over " << count << " entities, unpacking one component" << endl;

  ComponentHandle<Position> position;
  for (auto e : em.entities_with_components(position)) {
    (void)e;
  }
}

TEST_CASE_METHOD(BenchmarkFixture, "TestEntityIterationUnpackTwo") {
  int count = 10000000;
  for (int i = 0; i < count; i++) {
    auto e = em.create();
    e.assign<Position>();
    e.assign<Direction>();
  }

  AutoTimer t;
  cout << "iterating over " << count << " entities, unpacking two components" << endl;

  ComponentHandle<Position> position;
  ComponentHandle<Direction> direction;
  for (auto e : em.entities_with_components(position, direction)) {
    (void)e;
  }
}
