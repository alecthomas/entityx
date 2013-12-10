/*
 * Copyright (C) 2013 Antony Woods <antony@teamwoods.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Antony Woods <antony@teamwoods.org>
 */

#include "entityx/help/Timer.h"

namespace entityx {
namespace help {

Timer::Timer() {
  _start = std::chrono::system_clock::now();
}

Timer::~Timer() {
}

void Timer::restart() {
  _start = std::chrono::system_clock::now();
}

double Timer::elapsed() {
  return std::chrono::duration<double>(std::chrono::system_clock::now() - _start).count();
}

}  // namespace help
}  // namespace entityx
