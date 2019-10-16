/*
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include <algorithm>
#include "entityx/Entity.h"

namespace entityx {

const Entity::Id Entity::INVALID;
BaseComponent::Family BaseComponent::family_counter_ = 0;

void Entity::invalidate() {
  id_ = INVALID;
  manager_ = nullptr;
}

void Entity::destroy() {
  assert(valid());
  manager_->destroy(id_);
  invalidate();
}

std::bitset<entityx::MAX_COMPONENTS> Entity::component_mask() const {
  return manager_->component_mask(id_);
}

EntityManager::EntityManager(EventManager &event_manager) : event_manager_(event_manager) {
}

EntityManager::~EntityManager() {
  reset();
}

void EntityManager::reset() {
  for (Entity entity : entities_for_debugging()) entity.destroy();
  for (BasePool *pool : component_pools_) {
    if (pool) delete pool;
  }
  for (BaseComponentHelper *helper : component_helpers_) {
    if (helper) delete helper;
  }
  component_pools_.clear();
  component_helpers_.clear();
  entity_component_mask_.clear();
  entity_version_.clear();
  free_list_.clear();
  index_counter_ = 0;
}

EntityCreatedEvent::~EntityCreatedEvent() {}
EntityDestroyedEvent::~EntityDestroyedEvent() {}


}  // namespace entityx
