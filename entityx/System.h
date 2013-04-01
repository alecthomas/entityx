/**
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#pragma once


#include <stdint.h>
#include <cassert>
#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>
#include "entityx/config.h"
#include "entityx/Entity.h"
#include "entityx/Event.h"


namespace entityx {

/**
 * Base System class. Generally should not be directly used, instead see System<Derived>.
 */
class BaseSystem : boost::noncopyable {
 public:
  typedef uint64_t Family;

  virtual ~BaseSystem() {}

  /**
   * Called once all Systems have been added to the SystemManager.
   *
   * Typically used to set up event handlers.
   */
  virtual void configure(entityx::shared_ptr<EventManager> events) {}

  /**
   * Apply System behavior.
   *
   * Called every game step.
   */
  virtual void update(entityx::shared_ptr<EntityManager> entities, entityx::shared_ptr<EventManager> events, double dt) = 0;

  static Family family_counter_;

 protected:
};


/**
 * Use this class when implementing Systems.
 *
 * struct MovementSystem : public System<MovementSystem> {
 *   void update(entityx::shared_ptr<EntityManager> entities, EventManager &events, double dt) {
 *     // Do stuff to/with entities...
 *   }
 * }
 */
template <typename Derived>
class System : public BaseSystem {
 public:
  virtual ~System() {}

  static Family family() {
    static Family family = family_counter_++;
    return family;
  }
};


class SystemManager : public entityx::enable_shared_from_this<SystemManager>, boost::noncopyable {
 public:
  static entityx::shared_ptr<SystemManager> make(entityx::shared_ptr<EntityManager> entity_manager,
                  entityx::shared_ptr<EventManager> event_manager) {
    return entityx::shared_ptr<SystemManager>(new SystemManager(entity_manager, event_manager));
  }

  /**
   * Add a System to the SystemManager.
   *
   * Must be called before Systems can be used.
   *
   * eg.
   * entityx::shared_ptr<MovementSystem> movement = entityx::make_shared<MovementSystem>();
   * system.add(movement);
   */
  template <typename S>
  void add(entityx::shared_ptr<S> system) {
    systems_.insert(std::make_pair(S::family(), system));
  }

  /**
   * Add a System to the SystemManager.
   *
   * Must be called before Systems can be used.
   *
   * eg.
   * auto movement = system.add<MovementSystem>();
   */
  template <typename S, typename ... Args>
  entityx::shared_ptr<S> add(Args && ... args) {
    entityx::shared_ptr<S> s = entityx::make_shared<S>(args ...);
    add(s);
    return s;
  }

  /**
   * Retrieve the registered System instance, if any.
   *
   *   entityx::shared_ptr<CollisionSystem> collisions = systems.system<CollisionSystem>();
   *
   * @return System instance or empty shared_ptr<S>.
   */
  template <typename S>
  entityx::shared_ptr<S> system() {
    auto it = systems_.find(S::family());
    assert(it != systems_.end());
    return it == systems_.end()
        ? entityx::shared_ptr<S>()
        : entityx::static_pointer_cast<S>(it->second);
  }

  /**
   * Call the System::update() method for a registered system.
   */
  template <typename S>
  void update(double dt) {
    assert(initialized_ && "SystemManager::configure() not called");
    entityx::shared_ptr<S> s = system<S>();
    s->update(entity_manager_, event_manager_, dt);
  }

  /**
   * Configure the system. Call after adding all Systems.
   *
   * This is typically used to set up event handlers.
   */
  void configure();

 private:
  SystemManager(entityx::shared_ptr<EntityManager> entity_manager,
                entityx::shared_ptr<EventManager> event_manager) :
                entity_manager_(entity_manager),
                event_manager_(event_manager) {}

  bool initialized_ = false;
  entityx::shared_ptr<EntityManager> entity_manager_;
  entityx::shared_ptr<EventManager> event_manager_;
  boost::unordered_map<BaseSystem::Family, entityx::shared_ptr<BaseSystem>> systems_;
};

}
