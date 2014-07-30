/*
 * Copyright (C) 2014 Alec Thomas <alec@swapoff.org>
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
#include "entityx/config.h"

namespace entityx {

/**
 * A convenience class for instantiating an EventManager, EntityManager and
 * SystemManager.
 */
class EntityX {
 public:
  EntityX() : entities(events), systems(entities, events) {}

  EventManager events;
  EntityManager entities;
  SystemManager systems;
};

}  // namespace entityx
