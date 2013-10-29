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

void SystemManager::configure() {
  for (auto pair : systems_) {
    pair.second->configure(event_manager_);
  }
  initialized_ = true;
}

}
