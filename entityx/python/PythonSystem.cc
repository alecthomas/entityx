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
#include <boost/noncopyable.hpp>
#include <Python.h>
#include <cassert>
#include <string>
#include <iostream>
#include <sstream>
#include "entityx/python/PythonSystem.h"

namespace py = boost::python;

namespace entityx {
namespace python {


static const py::object None;


class PythonEntityXLogger {
public:
  PythonEntityXLogger() {}
  explicit PythonEntityXLogger(PythonSystem::LoggerFunction logger) : logger_(logger) {}
  ~PythonEntityXLogger() { flush(true); }

  void write(const std::string &text) {
    line_ += text;
    flush();
  }

private:
  void flush(bool force = false) {
    size_t offset;
    while ((offset = line_.find('\n')) != std::string::npos) {
      std::string text = line_.substr(0, offset);
      logger_(text);
      line_ = line_.substr(offset + 1);
    }
    if (force && line_.size()) {
      logger_(line_);
      line_ = "";
    }
  }

  PythonSystem::LoggerFunction logger_;
  std::string line_;
};


/**
 * Base class for Python entities.
 */
struct PythonEntity {
  explicit PythonEntity(ptr<EntityManager> entity_manager, Entity::Id id) : _entity(Entity(entity_manager, id)) {}  // NOLINT
  virtual ~PythonEntity() {}

  void destroy() {
    _entity.destroy();
  }

  operator Entity () const { return _entity; }

  virtual void update(float dt, int frame) {}

  Entity::Id _entity_id() const {
    return _entity.id();
  }

  Entity _entity;
};

static std::string PythonEntity_repr(const PythonEntity &entity) {
  std::stringstream repr;
  repr << "<Entity " << entity._entity.id().index() << "." << entity._entity.id().version() << ">";
  return repr.str();
}


static std::string Entity_Id_repr(Entity::Id id) {
  std::stringstream repr;
  repr << "<Entity::Id " << id.index() << "." << id.version() << ">";
  return repr.str();
}


// static std::string entity_repr(Entity entity) {
//   std::stringstream repr;
//   repr << "<Entity::Id " << entity.id().index() << "." << entity.id().version() << ">";
//   return repr.str();
// }

// static bool entity_eq(Entity left, Entity right) {
//   return left.id() == right.id();
// }


// A to-Python converter from Entity to PythonEntity.
struct EntityToPythonEntity {
  static PyObject *convert(Entity entity) {
    auto python = entity.component<PythonComponent>();
    assert(python && "Entity does not have a PythonComponent");
    return py::incref(python->object.ptr());
  }
};


Entity::Id EntityManager_configure(ptr<EntityManager> entity_manager, py::object self) {
  Entity entity = entity_manager->create();
  entity.assign<PythonComponent>(self);
  return entity.id();
}

BOOST_PYTHON_MODULE(_entityx) {
  py::to_python_converter<Entity, EntityToPythonEntity>();

  py::class_<PythonEntityXLogger>("Logger", py::no_init)
    .def("write", &PythonEntityXLogger::write);

  py::class_<BaseEvent, ptr<BaseEvent>, boost::noncopyable>("BaseEvent", py::no_init);

  py::class_<PythonEntity>("Entity", py::init<ptr<EntityManager>, Entity::Id>())
    .def_readonly("_entity_id", &PythonEntity::_entity_id)
    .def("update", &PythonEntity::update)
    .def("destroy", &PythonEntity::destroy)
    .def("__repr__", &PythonEntity_repr);

  py::class_<Entity::Id>("EntityId", py::no_init)
    .def_readonly("id", &Entity::Id::id)
    .def_readonly("index", &Entity::Id::index)
    .def_readonly("version", &Entity::Id::version)
    .def("__repr__", &Entity_Id_repr);

  py::class_<PythonComponent, ptr<PythonComponent>>("PythonComponent", py::init<py::object>())
    .def("assign_to", &assign_to<PythonComponent>)
    .def("get_component", &get_component<PythonComponent>)
    .staticmethod("get_component");

  py::class_<EntityManager, ptr<EntityManager>, boost::noncopyable>("EntityManager", py::no_init)
    .def("configure", &EntityManager_configure);

  void (EventManager::*emit)(const BaseEvent &) = &EventManager::emit;

  py::class_<EventManager, ptr<EventManager>, boost::noncopyable>("EventManager", py::no_init)
    .def("emit", emit);

  py::implicitly_convertible<PythonEntity, Entity>();
}


static void log_to_stderr(const std::string &text) {
  std::cerr << "python stderr: " << text << std::endl;
}

static void log_to_stdout(const std::string &text) {
  std::cout << "python stdout: " << text << std::endl;
}

// PythonSystem below here

bool PythonSystem::initialized_ = false;

PythonSystem::PythonSystem(ptr<EntityManager> entity_manager)
    : entity_manager_(entity_manager), stdout_(log_to_stdout), stderr_(log_to_stderr) {
  if (!initialized_) {
    initialize_python_module();
  }
  Py_Initialize();
  if (!initialized_) {
    init_entityx();
    initialized_ = true;
  }
}

PythonSystem::~PythonSystem() {
  try {
    py::object entityx = py::import("_entityx");
    entityx.attr("_entity_manager").del();
    entityx.attr("_event_manager").del();
    py::object sys = py::import("sys");
    sys.attr("stdout").del();
    sys.attr("stderr").del();
    py::object gc = py::import("gc");
    gc.attr("collect")();
  } catch(...) {
    PyErr_Print();
    PyErr_Clear();
    throw;
  }
  // FIXME: It would be good to do this, but it is not supported by boost::python:
  // http://www.boost.org/doc/libs/1_53_0/libs/python/todo.html#pyfinalize-safety
  // Py_Finalize();
}

void PythonSystem::add_installed_library_path() {
  add_path(ENTITYX_INSTALLED_PYTHON_PACKAGE_DIR);
}

void PythonSystem::add_path(const std::string &path) {
  python_paths_.push_back(path);
}

void PythonSystem::initialize_python_module() {
  assert(PyImport_AppendInittab("_entityx", init_entityx) != -1 && "Failed to initialize _entityx Python module");
}

void PythonSystem::configure(ptr<EventManager> event_manager) {
  event_manager->subscribe<EntityDestroyedEvent>(*this);
  event_manager->subscribe<ComponentAddedEvent<PythonComponent>>(*this);

  try {
    py::object main_module = py::import("__main__");
    py::object main_namespace = main_module.attr("__dict__");

    // Initialize logging.
    py::object sys = py::import("sys");
    sys.attr("stdout") = PythonEntityXLogger(stdout_);
    sys.attr("stderr") = PythonEntityXLogger(stderr_);

    // Add paths to interpreter sys.path
    for (auto path : python_paths_) {
      py::str dir = path.c_str();
      sys.attr("path").attr("insert")(0, dir);
    }

    py::object entityx = py::import("_entityx");
    entityx.attr("_entity_manager") = entity_manager_.lock();
    entityx.attr("_event_manager") = event_manager;
  } catch(...) {
    PyErr_Print();
    PyErr_Clear();
    throw;
  }
}

void PythonSystem::update(ptr<EntityManager> entity_manager, ptr<EventManager> event_manager, double dt) {
  for (auto entity : entity_manager->entities_with_components<PythonComponent>()) {
    ptr<PythonComponent> python = entity.component<PythonComponent>();

    try {
      python->object.attr("update")(dt, frame_);
    } catch(...) {
      PyErr_Print();
      PyErr_Clear();
      throw;
    }
  }
  frame_++;
}

void PythonSystem::log_to(LoggerFunction stdout, LoggerFunction stderr) {
  stdout_ = stdout;
  stderr_ = stderr;
}

void PythonSystem::receive(const EntityDestroyedEvent &event) {
  for (auto proxy : event_proxies_) {
    proxy->delete_receiver(event.entity);
  }
}

void PythonSystem::receive(const ComponentAddedEvent<PythonComponent> &event) {
  // If the component was created in C++ it won't have a Python object
  // associated with it. Create one.
  if (!event.component->object) {
    py::object module = py::import(event.component->module.c_str());
    py::object cls = module.attr(event.component->cls.c_str());
    py::object from_raw_entity = cls.attr("_from_raw_entity");
    if (py::len(event.component->args) == 0) {
      event.component->object = from_raw_entity(event.entity.id());
    } else {
      py::list args;
      args.append(event.entity.id());
      args.extend(event.component->args);
      event.component->object = from_raw_entity(*py::tuple(args));
    }
  }

  for (auto proxy : event_proxies_) {
    if (proxy->can_send(event.component->object)) {
      proxy->add_receiver(event.entity);
    }
  }
}

}  // namespace python
}  // namespace entityx
