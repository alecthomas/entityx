/*
 * Copyright (C) 2013 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

// http://docs.python.org/2/extending/extending.html
#include <Python.h>
#include <cassert>
#include <vector>
#include <string>
#include <gtest/gtest.h>
#include <boost/python.hpp>
#include "entityx/Entity.h"
#include "entityx/Event.h"
#include "entityx/python/PythonSystem.h"

using namespace std;
using namespace boost;
namespace py = boost::python;
using namespace entityx;
using namespace entityx::python;


struct Position : public Component<Position> {
  Position(float x = 0.0, float y = 0.0) : x(x), y(y) {}

  float x, y;
};


struct Direction : public Component<Direction> {
  Direction(float x = 0.0, float y = 0.0) : x(x), y(y) {}

  float x, y;
};


struct CollisionEvent : public Event<CollisionEvent> {
  CollisionEvent(Entity a, Entity b) : a(a), b(b) {}

  Entity a, b;
};


struct CollisionEventProxy : public PythonEventProxy, public Receiver<CollisionEvent> {
  CollisionEventProxy() : PythonEventProxy("on_collision") {}

  void receive(const CollisionEvent &event) {
    for (auto entity : entities) {
      auto py_entity = entity.component<PythonComponent>();
      if (entity == event.a || entity == event.b) {
        py_entity->object.attr("on_collision")(event);
      }
    }
  }
};


BOOST_PYTHON_MODULE(entityx_python_test) {
  py::class_<Position, entityx::shared_ptr<Position>>("Position", py::init<py::optional<float, float>>())
    .def("assign_to", &assign_to<Position>)
    .def("get_component", &get_component<Position>)
    .staticmethod("get_component")
    .def_readwrite("x", &Position::x)
    .def_readwrite("y", &Position::y);
  py::class_<Direction, entityx::shared_ptr<Direction>>("Direction", py::init<py::optional<float, float>>())
    .def("assign_to", &assign_to<Direction>)
    .def("get_component", &get_component<Direction>)
    .staticmethod("get_component")
    .def_readwrite("x", &Direction::x)
    .def_readwrite("y", &Direction::y);
  py::class_<CollisionEvent>("Collision", py::init<Entity, Entity>())
    .def_readonly("a", &CollisionEvent::a)
    .def_readonly("b", &CollisionEvent::b);
}


class PythonSystemTest : public ::testing::Test {
protected:
  PythonSystemTest() {
    assert(PyImport_AppendInittab("entityx_python_test", initentityx_python_test) != -1 && "Failed to initialize entityx_python_test Python module");
  }

  void SetUp() {
    ev = EventManager::make();
    em = EntityManager::make(ev);
    vector<string> paths;
    paths.push_back(ENTITYX_PYTHON_TEST_DATA);
    system = entityx::make_shared<PythonSystem>(em);
    system->add_paths(paths);
    if (!initialized) {
      initentityx_python_test();
      initialized = true;
    }
    system->add_event_proxy<CollisionEvent>(ev, entityx::make_shared<CollisionEventProxy>());
    system->configure(ev);
  }

  void TearDown() {
    system.reset();
    em.reset();
    ev.reset();
  }

  entityx::shared_ptr<PythonSystem> system;
  entityx::shared_ptr<EventManager> ev;
  entityx::shared_ptr<EntityManager> em;
  static bool initialized;
};

bool PythonSystemTest::initialized = false;


TEST_F(PythonSystemTest, TestSystemUpdateCallsEntityUpdate) {
  try {
    Entity e = em->create();
    auto script = e.assign<PythonComponent>("entityx.tests.update_test", "UpdateTest");
    ASSERT_FALSE(py::extract<bool>(script->object.attr("updated")));
    system->update(em, ev, 0.1);
    ASSERT_TRUE(py::extract<bool>(script->object.attr("updated")));
  } catch (...) {
    PyErr_Print();
    PyErr_Clear();
    ASSERT_FALSE(true) << "Python exception.";
  }
}


TEST_F(PythonSystemTest, TestComponentAssignmentCreationInPython) {
  try {
    Entity e = em->create();
    auto script = e.assign<PythonComponent>("entityx.tests.assign_test", "AssignTest");
    ASSERT_TRUE(bool(e.component<Position>()));
    ASSERT_TRUE(script->object);
    ASSERT_TRUE(script->object.attr("test_assign_create"));
    script->object.attr("test_assign_create")();
    auto position = e.component<Position>();
    ASSERT_TRUE(bool(position));
    ASSERT_EQ(position->x, 1.0);
    ASSERT_EQ(position->y, 2.0);
  } catch (...) {
    PyErr_Print();
    PyErr_Clear();
    ASSERT_FALSE(true) << "Python exception.";
  }
}


TEST_F(PythonSystemTest, TestComponentAssignmentCreationInCpp) {
  try {
    Entity e = em->create();
    e.assign<Position>(2, 3);
    auto script = e.assign<PythonComponent>("entityx.tests.assign_test", "AssignTest");
    ASSERT_TRUE(bool(e.component<Position>()));
    ASSERT_TRUE(script->object);
    ASSERT_TRUE(script->object.attr("test_assign_existing"));
    script->object.attr("test_assign_existing")();
    auto position = e.component<Position>();
    ASSERT_TRUE(bool(position));
    ASSERT_EQ(position->x, 3.0);
    ASSERT_EQ(position->y, 4.0);
  } catch (...) {
    PyErr_Print();
    PyErr_Clear();
    ASSERT_FALSE(true) << "Python exception.";
  }
}


TEST_F(PythonSystemTest, TestEntityConstructorArgs) {
  try {
    Entity e = em->create();
    auto script = e.assign<PythonComponent>("entityx.tests.constructor_test", "ConstructorTest", 4.0, 5.0);
    auto position = e.component<Position>();
    ASSERT_TRUE(bool(position));
    ASSERT_EQ(position->x, 4.0);
    ASSERT_EQ(position->y, 5.0);
  } catch (...) {
    PyErr_Print();
    PyErr_Clear();
    ASSERT_FALSE(true) << "Python exception.";
  }
}


TEST_F(PythonSystemTest, TestEventDelivery) {
  try {
    Entity f = em->create();
    Entity e = em->create();
    Entity g = em->create();
    auto scripte = e.assign<PythonComponent>("entityx.tests.event_test", "EventTest");
    auto scriptf = f.assign<PythonComponent>("entityx.tests.event_test", "EventTest");
    ASSERT_FALSE(scripte->object.attr("collided"));
    ASSERT_FALSE(scriptf->object.attr("collided"));
    ev->emit<CollisionEvent>(f, g);
    ASSERT_TRUE(scriptf->object.attr("collided"));
    ASSERT_FALSE(scripte->object.attr("collided"));
    ev->emit<CollisionEvent>(e, f);
    ASSERT_TRUE(scriptf->object.attr("collided"));
    ASSERT_TRUE(scripte->object.attr("collided"));
  } catch (...) {
    PyErr_Print();
    PyErr_Clear();
    ASSERT_FALSE(true) << "Python exception.";
  }
}


TEST_F(PythonSystemTest, TestDeepEntitySubclass) {
  try {
    Entity e = em->create();
    auto script = e.assign<PythonComponent>("entityx.tests.deep_subclass_test", "DeepSubclassTest");
    ASSERT_TRUE(script->object.attr("test_deep_subclass"));
    script->object.attr("test_deep_subclass")();

    Entity e2 = em->create();
    auto script2 = e2.assign<PythonComponent>("entityx.tests.deep_subclass_test", "DeepSubclassTest2");
    ASSERT_TRUE(script2->object.attr("test_deeper_subclass"));
    script2->object.attr("test_deeper_subclass")();
  } catch (...) {
    PyErr_Print();
    PyErr_Clear();
    ASSERT_FALSE(true) << "Python exception.";
  }
}


TEST_F(PythonSystemTest, TestEntityCreationFromPython) {
  try {
    py::object test = py::import("entityx.tests.create_entities_from_python_test");
    test.attr("create_entities_from_python_test")();
  } catch (...) {
    PyErr_Print();
    PyErr_Clear();
    ASSERT_FALSE(true) << "Python exception.";
  }
}
