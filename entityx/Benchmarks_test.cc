#include <vector>
#include <gtest/gtest.h>
#include <boost/timer/timer.hpp>
#include "entityx/Entity.h"

using namespace std;
using namespace entityx;

class BenchmarksTest : public ::testing::Test {
protected:
 BenchmarksTest() : em(ev) {}

 EventManager ev;
 EntityManager em;
};


TEST_F(BenchmarksTest, TestCreateEntities) {
  boost::timer::auto_cpu_timer t;

  uint64_t count = 10000000L;
  for (uint64_t i = 0; i < count; i++) {
    em.create();
  }
}


TEST_F(BenchmarksTest, TestDestroyEntities) {
  uint64_t count = 10000000L;
  vector<Entity> entities;
  for (uint64_t i = 0; i < count; i++) {
    entities.push_back(em.create());
  }

  boost::timer::auto_cpu_timer t;

  for (auto e : entities) {
    e.destroy();
  }
}

struct Listener : public Receiver<Listener> {
  void receive(const EntityCreatedEvent &event) {}
  void receive(const EntityDestroyedEvent &event) {}
};

TEST_F(BenchmarksTest, TestCreateEntitiesWithListener) {
  Listener listen;
  ev.subscribe<EntityCreatedEvent>(listen);

  boost::timer::auto_cpu_timer t;
  uint64_t count = 10000000L;
  vector<Entity> entities;
  for (uint64_t i = 0; i < count; i++) {
    entities.push_back(em.create());
  }
}

TEST_F(BenchmarksTest, TestDestroyEntitiesWithListener) {
  Listener listen;
  ev.subscribe<EntityDestroyedEvent>(listen);

  uint64_t count = 10000000L;
  vector<Entity> entities;
  for (uint64_t i = 0; i < count; i++) {
    entities.push_back(em.create());
  }

  boost::timer::auto_cpu_timer t;

  for (auto e : entities) {
    e.destroy();
  }
}

