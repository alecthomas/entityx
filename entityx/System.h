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


#include <cstdint>
#include <unordered_map>
#include <utility>
#include <cassert>
#include "entityx/config.h"
#include "entityx/Entity.h"
#include "entityx/Event.h"
#include "entityx/help/NonCopyable.h"


namespace entityx {


class SystemManager;


/**
 * Base System class. Generally should not be directly used, instead see System<Derived>.
 */
class BaseSystem : entityx::help::NonCopyable {
 public:
  typedef size_t Family;

  virtual ~BaseSystem();

  /**
   * Called once all Systems have been added to the SystemManager.
   *
   * Typically used to set up event handlers.
   */
  virtual void configure(EventManager &events) {}

  /**
   * Apply System behavior.
   *
   * Called every game step.
   */
  virtual void update(EntityManager &entities, EventManager &events, TimeDelta dt) = 0;

  static Family family_counter_;

 protected:
};


/**
 * Use this class when implementing Systems.
 *
 * struct MovementSystem : public System<MovementSystem> {
 *   void update(EntityManager &entities, EventManager &events, TimeDelta dt) {
 *     // Do stuff to/with entities...
 *   }
 * }
 */
template <typename Derived>
class System : public BaseSystem {
 public:
  virtual ~System() {}

private:
  friend class SystemManager;

  static Family family() {
    static Family family = family_counter_++;
    return family;
  }
};


class SystemManager : entityx::help::NonCopyable {
 public:
  SystemManager(EntityManager &entity_manager,
                EventManager &event_manager) :
                entity_manager_(entity_manager),
                event_manager_(event_manager) {}

  /**
   * Add a System to the SystemManager.
   *
   * Must be called before Systems can be used.
   *
   * eg.
   * std::shared_ptr<MovementSystem> movement = entityx::make_shared<MovementSystem>();
   * system.add(movement);
   */
  template <typename S>
  void add(std::shared_ptr<S> system) {
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
  std::shared_ptr<S> add(Args && ... args) {
    std::shared_ptr<S> s(new S(std::forward<Args>(args) ...));
    add(s);
    return s;
  }

  /**
   * Retrieve the registered System instance, if any.
   *
   *   std::shared_ptr<CollisionSystem> collisions = systems.system<CollisionSystem>();
   *
   * @return System instance or empty shared_std::shared_ptr<S>.
   */
  template <typename S>
  std::shared_ptr<S> system() {
    auto it = systems_.find(S::family());
    assert(it != systems_.end());
    return it == systems_.end()
        ? std::shared_ptr<S>()
        : std::shared_ptr<S>(std::static_pointer_cast<S>(it->second));
  }

  /**
   * Call the System::update() method for a registered system.
   */
  template <typename S>
  void update(TimeDelta dt) {
    assert(initialized_ && "SystemManager::configure() not called");
    std::shared_ptr<S> s = system<S>();
    s->update(entity_manager_, event_manager_, dt);
  }

  /**
   * Call System::update() on all registered systems.
   *
   * The order which the registered systems are updated is arbitrary but consistent,
   * meaning the order which they will be updated cannot be specified, but that order
   * will stay the same as long no systems are added or removed.
   *
   * If the order in which systems update is important, use SystemManager::update()
   * to manually specify the update order. EntityX does not yet support a way of
   * specifying priority for update_all().
   */
  void update_all(TimeDelta dt);

  /**
   * Configure the system. Call after adding all Systems.
   *
   * This is typically used to set up event handlers.
   */
  void configure();

 private:
  bool initialized_ = false;
  EntityManager &entity_manager_;
  EventManager &event_manager_;
  std::unordered_map<BaseSystem::Family, std::shared_ptr<BaseSystem>> systems_;
};

}  // namespace entityx
