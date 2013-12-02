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

#include "entityx/Entity.h"
#include "entityx/Event.h"
#include "entityx/System.h"
#include "entityx/help/Timer.h"

namespace entityx {

class Manager {
 public:
  virtual ~Manager() {}

  /**
   * Call start() to initialize the Manager.
   */
  void start();


  /**
   * Run the main loop. To explicitly manage your own main loop use step(dt);
   */
  void run();

  /**
   * Step the system by dt.
   * @param dt Delta time since last frame.
   */
  void step(double dt);

  /**
   * Stop the manager.
   */
  void stop();

 protected:
  Manager() :
    event_manager(EventManager::make()),
    entity_manager(EntityManager::make(event_manager)),
    system_manager(SystemManager::make(entity_manager, event_manager)) {}

  /**
   * Configure the world.
   *
   * This is called once on Manager initialization. It is typically used to add Systems to the world, load permanent
   * resources, global configuration, etc.
   */
  virtual void configure() = 0;

  /**
   * Initialize the entities and events in the world.
   *
   * Typically used to create initial entities, setup event handlers, and so on.
   */
  virtual void initialize() = 0;

  /**
   * Update the world.
   *
   * Typically this is where you would call update() on all Systems in the world.
   */
  virtual void update(double dt) = 0;

  ptr<EventManager> event_manager;
  ptr<EntityManager> entity_manager;
  ptr<SystemManager> system_manager;

 private:
  help::Timer timer_;
  bool running_ = false;
};

}  // namespace entityx
