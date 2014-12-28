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
#include "entityx/Entity.hh"

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

EntityManager::EntityManager() {
}

EntityManager::~EntityManager() {
  reset();
}

void EntityManager::reset() {
  reset_callbacks();
  for (Entity entity : entities_for_debugging()) entity.destroy();
  for (BasePool *pool : component_pools_) {
    if (pool) delete pool;
  }
  component_pools_.clear();
  entity_component_mask_.clear();
  entity_version_.clear();
  free_list_.clear();
  index_counter_ = 0;
}

}  // namespace entityx
