#define CATCH_CONFIG_MAIN

#include <iostream>
#include <vector>
#include "entityx/help/catch.hpp"
#include "entityx/help/Timer.hh"
#include "entityx/entityx.hh"

using namespace std;
using namespace entityx;

using std::uint64_t;


struct AutoTimer {
  ~AutoTimer() {
    cout << timer_.elapsed() << " seconds elapsed" << endl;
  }

private:
  entityx::help::Timer timer_;
};

struct Position : public Component<Position> {
};


struct Direction : public Component<Direction> {
};


struct BenchmarkFixture {
  BenchmarkFixture() {}

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
  int created = 0;
  em.on_entity_created([&](Entity entity) { created++; });

  int count = 10000000L;

  AutoTimer t;
  cout << "creating " << count << " entities while notifying on_entity_created()" << endl;

  vector<Entity> entities;
  for (int i = 0; i < count; i++) {
    entities.push_back(em.create());
  }

  REQUIRE(entities.size() == count);
  REQUIRE(created == count);
}

TEST_CASE_METHOD(BenchmarkFixture, "TestDestroyEntitiesWithListener") {
  int count = 10000000;
  vector<Entity> entities;
  for (int i = 0; i < count; i++) {
    entities.push_back(em.create());
  }

  int destroyed = 0;
  em.on_entity_destroyed([&](Entity) { destroyed++; });

  AutoTimer t;
  cout << "destroying " << count << " entities while notifying on_entity_destroyed()" << endl;

  for (auto &e : entities) {
    e.destroy();
  }

  REQUIRE(entities.size() == count);
  REQUIRE(destroyed == count);
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
