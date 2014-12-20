/*
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include "entityx/System.h"

namespace entityx {

BaseSystem::Family BaseSystem::family_counter_;

BaseSystem::~BaseSystem() {
}

void SystemManager::update_all(TimeDelta dt) {
  assert(initialized_ && "SystemManager::configure() not called");
  for (auto &pair : systems_) {
    pair.second->update(entity_manager_, event_manager_, dt);
  }
}

void SystemManager::configure() {
  for (auto &pair : systems_) {
    pair.second->configure(event_manager_);
  }
  initialized_ = true;
}

}  // namespace entityx
