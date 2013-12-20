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
  manager_.reset();
}

void Entity::destroy() {
  assert(valid());
  manager_.lock()->destroy(id_);
  invalidate();
}

std::bitset<entityx::MAX_COMPONENTS> Entity::component_mask() const {
  return manager_.lock()->component_mask(id_);
}

EntityManager::EntityManager(ptr<EventManager> event_manager) : event_manager_(event_manager) {
}

EntityManager::~EntityManager() {
}

void EntityManager::destroy_all() {
  entity_components_.clear();
  entity_component_mask_.clear();
  entity_version_.clear();
  free_list_.clear();
}


}  // namespace entityx
