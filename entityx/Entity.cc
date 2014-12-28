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
BasePool*(*BaseComponent::pool_factory_[MAX_COMPONENTS])();


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
  BaseComponent::Family components = BaseComponent::family_counter_;
  on_component_added_.resize(components);
  on_component_removed_.resize(components);
  BaseComponent::Family i;
  for (i = 0; i < components; i++) {
    BasePool *pool = BaseComponent::pool_factory_[i]();
    component_pools_[i] = pool;
  }
  for (; i < MAX_COMPONENTS; i++) {
    BaseComponent::pool_factory_[i] = nullptr;
    component_pools_[i] = nullptr;
  }
}

EntityManager::~EntityManager() {
  reset();
}

void EntityManager::reset() {
  reset_callbacks();
  for (Entity entity : entities_for_debugging()) entity.destroy();
  for (BaseComponent::Family i = 0; i < BaseComponent::family_counter_; i++) {
    delete component_pools_[i];
    component_pools_[i] = nullptr;
  }
  entity_component_mask_.clear();
  entity_version_.clear();
  free_list_.clear();
  index_counter_ = 0;
}

}  // namespace entityx
