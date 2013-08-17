# Python Scripting System for EntityX (α Alpha)

This system adds the ability to extend entity logic with Python scripts. The goal is to allow ad-hoc behaviour to be assigned to entities, in contract to the more pure entity-component system approach.

## Limitations

Planned features that are currently unimplemented:

- Emitting events from Python.

## Concepts

- Python scripts are attached to entities with `PythonComponent`.
- Events are proxied directly to Python entities via `PythonEventProxy` objects.
- `PythonSystem` manages scripted entity lifecycle and event delivery.

## Overview

To add scripting support to your system, something like the following steps should be followed:

1. Expose C++ `Component` and `Event` classes to Python with `BOOST_PYTHON_MODULE`.
2. Initialize the module with `PyImport_AppendInittab`.
3. Create a Python package.
4. Add classes to the package, inheriting from `entityx.Entity` and using the `entityx.Component` descriptor to assign components.
5. Create a `PythonSystem`, passing in the list of paths to add to Python's import search path.
6. Optionally attach any event proxies.
7. Create an entity and associate it with a Python script by assigning `PythonComponent`, passing it the package name, class name, and any constructor arguments.

## Interfacing with Python

`entityx::python` primarily uses standard `boost::python` to interface with Python, with some helper classes and functions.

### Exposing C++ Components to Python

In most cases, this should be pretty simple. Given a component, provide a `boost::python` class definition, with two extra methods defined with EntityX::Python helper functions `assign_to<Component>` and `get_component<Component>`. These are used from Python to assign Python-created components to an entity and to retrieve existing components from an entity, respectively.

Here's an example:

```c++
namespace py = boost::python;

struct Position : public Component<Position> {
  Position(float x = 0.0, float y = 0.0) : x(x), y(y) {}

  float x, y;
};

void export_position_to_python() {
  py::class_<PythonPosition, entityx::shared_ptr<PythonPosition>>("Position", py::init<py::optional<float, float>>())
    .def("assign_to", &entityx::python::assign_to<Position>)
    .def("get_component", &entityx::python::get_component<Position>)
    .staticmethod("get_component")
    .def_readwrite("x", &PythonPosition::x)
    .def_readwrite("y", &PythonPosition::y);
}

BOOST_PYTHON_MODULE(mygame) {
  export_position_to_python();
}
```

### Delivering events to Python entities

Unlike in C++, where events are typically handled by systems, EntityX::Python
explicitly provides support for sending events to entities. To bridge this gap
use the `PythonEventProxy` class to receive C++ events and proxy them to
Python entities.

The class takes a single parameter, which is the name of the attribute on a
Python entity. If this attribute exists, the entity will be added to
`PythonEventProxy::entities (std::list<Entity>)`, so that matching entities
will be accessible from any event handlers.

This checking is performed in `PythonEventProxy::can_send()`, and can be
overridden, but further checking can also be done in the event `receive()`
method.

A helper template class called `BroadcastPythonEventProxy<Event>` is provided
that will broadcast events to any entity with the corresponding handler method.

To implement more refined logic, subclass `PythonEventProxy` and operate on
the protected member `entities`. Here's a collision example, where the proxy
only delivers collision events to the colliding entities themselves:

```c++
struct CollisionEvent : public Event<CollisionEvent> {
  CollisionEvent(Entity a, Entity b) : a(a), b(b) {}

  Entity a, b;
};

struct CollisionEventProxy : public PythonEventProxy, public Receiver<CollisionEvent> {
  CollisionEventProxy() : PythonEventProxy("on_collision") {}

  void receive(const CollisionEvent &event) {
    // "entities" is a protected data member, populated by
    // PythonSystem, with Python entities that pass can_send().
    for (auto entity : entities) {
      auto py_entity = entity.template component<PythonComponent>();
      if (entity == event.a || entity == event.b) {
        py_entity->object.attr(handler_name.c_str())(event);
      }
    }
  }
};

void export_collision_event_to_python() {
  py::class_<CollisionEvent>("Collision", py::init<Entity, Entity>())
    .def_readonly("a", &CollisionEvent::a)
    .def_readonly("b", &CollisionEvent::b);
}


BOOST_PYTHON_MODULE(mygame) {
  export_position_to_python();
  export_collision_event_to_python();
}
```

### Initialization

Finally, initialize the `mygame` module once, before using `PythonSystem`, with something like this:

```c++
// This should only be performed once, at application initialization time.
CHECK(PyImport_AppendInittab("mygame", initmygame) != -1)
  << "Failed to initialize mygame Python module";
```

Then create and destroy `PythonSystem` as necessary:

```c++
// Initialize the PythonSystem.
vector<string> paths;
paths.push_back(MYGAME_PYTHON_PATH);
// +any other Python paths...
auto script_system = make_shared<PythonSystem>(paths);

// Add any Event proxies.
script_system->add_event_proxy<CollisionEvent>(
    ev, entityx::make_shared<CollisionEventProxy>());
```
