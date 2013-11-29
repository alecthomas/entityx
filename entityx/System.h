/*
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#pragma once


#include <unordered_map>
#include <stdint.h>
#include <cassert>
#include "entityx/config.h"
#include "entityx/Entity.h"
#include "entityx/Event.h"
#include "entityx/help/NonCopyable.h"


namespace entityx {

/**
 * Base System class. Generally should not be directly used, instead see System<Derived>.
 */
class BaseSystem : entityx::help::NonCopyable {
 public:
  typedef uint64_t Family;

  virtual ~BaseSystem() {}

  /**
   * Called once all Systems have been added to the SystemManager.
   *
   * Typically used to set up event handlers.
   */
  virtual void configure(ptr<EventManager> events) {}

  /**
   * Apply System behavior.
   *
   * Called every game step.
   */
  virtual void update(ptr<EntityManager> entities, ptr<EventManager> events, double dt) = 0;

  static Family family_counter_;

 protected:
};


/**
 * Use this class when implementing Systems.
 *
 * struct MovementSystem : public System<MovementSystem> {
 *   void update(ptr<EntityManager> entities, EventManager &events, double dt) {
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


class SystemManager : entityx::help::NonCopyable, public enable_shared_from_this<SystemManager> {
 public:
  SystemManager(ptr<EntityManager> entity_manager,
                ptr<EventManager> event_manager) :
                entity_manager_(entity_manager),
                event_manager_(event_manager) {}

  static ptr<SystemManager> make(ptr<EntityManager> entity_manager,
                  ptr<EventManager> event_manager) {
    return ptr<SystemManager>(new SystemManager(entity_manager, event_manager));
  }

  /**
   * Add a System to the SystemManager.
   *
   * Must be called before Systems can be used.
   *
   * eg.
   * ptr<MovementSystem> movement = entityx::make_shared<MovementSystem>();
   * system.add(movement);
   */
  template <typename S>
  void add(ptr<S> system) {
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
  ptr<S> add(Args && ... args) {
    ptr<S> s(new S(args ...));
    add(s);
    return s;
  }

  /**
   * Retrieve the registered System instance, if any.
   *
   *   ptr<CollisionSystem> collisions = systems.system<CollisionSystem>();
   *
   * @return System instance or empty shared_ptr<S>.
   */
  template <typename S>
  ptr<S> system() {
    auto it = systems_.find(S::family());
    assert(it != systems_.end());
    return it == systems_.end()
        ? ptr<S>()
        : ptr<S>(static_pointer_cast<S>(it->second));
  }

  /**
   * Call the System::update() method for a registered system.
   */
  template <typename S>
  void update(double dt) {
    assert(initialized_ && "SystemManager::configure() not called");
    ptr<S> s = system<S>();
    s->update(entity_manager_, event_manager_, dt);
  }

  /**
   * Configure the system. Call after adding all Systems.
   *
   * This is typically used to set up event handlers.
   */
  void configure();

 private:
  bool initialized_ = false;
  ptr<EntityManager> entity_manager_;
  ptr<EventManager> event_manager_;
  std::unordered_map<BaseSystem::Family, ptr<BaseSystem>> systems_;
};

}  // namespace entityx
