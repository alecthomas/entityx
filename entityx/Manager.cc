/*
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include "Manager.h"

namespace entityx {

void Manager::start() {
  configure();
  system_manager->configure();
  initialize();
}

void Manager::run() {
  running_ = true;
  double dt;
  timer_.restart();
  while (running_) {
    dt = timer_.elapsed();
    timer_.restart();
    update(dt);
  }
}

void Manager::step(double dt) {
  update(dt);
}


void Manager::stop() {
  running_ = false;
}

}  // namespace entityx
