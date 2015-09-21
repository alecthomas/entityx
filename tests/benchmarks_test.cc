#define CATCH_CONFIG_MAIN

#include <iostream>
#include <vector>
#include <chrono>
#include "./3rdparty/catch.hh"
#include "entityx/entityx.hh"

using namespace std;
using namespace entityx;

using std::uint64_t;


class Timer {
public:
  Timer() {
    restart();
  }
  ~Timer() {}

  void restart() {
    _start = std::chrono::system_clock::now();
  }

  double elapsed() {
    return std::chrono::duration<double>(std::chrono::system_clock::now() - _start).count();
  }
private:
  std::chrono::time_point<std::chrono::system_clock> _start;
};



struct AutoTimer {
  ~AutoTimer() {
    cout << timer_.elapsed() << " seconds elapsed" << endl;
  }

private:
  Timer timer_;
};

struct Position {
  float x, y;
};


struct Direction {
  float x, y;
};


typedef Components<Position, Direction> GameComponents;
typedef EntityX<GameComponents, ColumnStorage<GameComponents, 10000000L>> EntityManager;
typedef EntityX<GameComponents, ColumnStorage<GameComponents, 10000000L>, OBSERVABLE> EntityManagerWithListener;
template <typename C> using Component = EntityManager::Component<C>;
using Entity = EntityManager::Entity;


struct BenchmarkFixture {
  BenchmarkFixture() {}

  EntityManager em;
};


TEST_CASE_METHOD(BenchmarkFixture, "TestCreateEntities") {
  {
    AutoTimer t;

    uint64_t count = 10000000L;
    cout << "creating " << count << " entities" << endl;

    for (uint64_t i = 0; i < count; i++)
      em.create();
  }
}


TEST_CASE_METHOD(BenchmarkFixture, "TestCreateManyEntities") {
  {
    AutoTimer t;

    uint64_t count = 10000000L;
    cout << "creating " << count << " entities with create_many()" << endl;

    em.create_many(count);
  }
}


TEST_CASE_METHOD(BenchmarkFixture, "TestDestroyEntities") {
  uint64_t count = 10000000L;
  vector<Entity> entities(em.create_many(count));

  {
    AutoTimer t;
    cout << "destroying " << count << " entities" << endl;

    for (auto e : entities) {
      e.destroy();
    }
  }
}

struct ListenerBenchmarkFixture {
  ListenerBenchmarkFixture() {}

  EntityManagerWithListener em;
};


TEST_CASE_METHOD(ListenerBenchmarkFixture, "TestCreateEntitiesWithListener") {
  int created = 0;
  em.on_entity_created([&](EntityManagerWithListener::Entity) { created++; });

  int count = 10000000L;

  AutoTimer t;
  cout << "creating " << count << " entities while notifying on_entity_created()" << endl;

  vector<EntityManagerWithListener::Entity> entities;
  for (int i = 0; i < count; i++) {
    entities.push_back(em.create());
  }

  REQUIRE(entities.size() == count);
  REQUIRE(created == count);
}

TEST_CASE_METHOD(ListenerBenchmarkFixture, "TestCreateManyEntitiesWithListener") {
  int created = 0;
  em.on_entity_created([&](EntityManagerWithListener::Entity) { created++; });

  int count = 10000000L;

  AutoTimer t;
  cout << "creating " << count << " entities with create_many() while notifying on_entity_created()" << endl;

  vector<EntityManagerWithListener::Entity> entities = em.create_many(count);

  REQUIRE(entities.size() == count);
  REQUIRE(created == count);
}

TEST_CASE_METHOD(ListenerBenchmarkFixture, "TestDestroyEntitiesWithListener") {
  int count = 10000000;
  vector<EntityManagerWithListener::Entity> entities;
  for (int i = 0; i < count; i++) {
    entities.push_back(em.create());
  }

  int destroyed = 0;
  em.on_entity_destroyed([&](EntityManagerWithListener::Entity) { destroyed++; });
  em.on_component_added<Position>([&](EntityManagerWithListener::Entity, EntityManagerWithListener::Component<Position> position) {

  });

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

  Component<Position> position;
  for (auto e : em.entities_with_components(position)) {
    (void)e;
  }
}

TEST_CASE_METHOD(BenchmarkFixture, "TestEntityCreationIterationDeletionRepeatedly") {
  AutoTimer t;
  cout << "looping 10000 times creating and deleting a random number of entities" << endl;

  for (int i = 0; i < 10000; i++) {
    for (int j = 0; j < 10000; j++) {
      Entity e = em.create();
      e.assign<Position>();
    }
    Component<Position> position;
    for (auto e : em.entities_with_components(position)) {
      if (rand() % 2 == 0) e.destroy();
    }
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

  Component<Position> position;
  Component<Direction> direction;
  for (auto e : em.entities_with_components(position, direction)) {
    (void)e;
  }
}
