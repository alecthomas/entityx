/*
 * Copyright (C) 2013 Antony Woods <antony@teamwoods.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Antony Woods <antony@teamwoods.org>
 */
#pragma once

#include <chrono>

namespace entityx {
namespace help {

class Timer {
public:
  Timer() {
    restart();
  }
  ~Timer() {}

  void restart() {
    _start = std::chrono::system_clock::now();
  }

  double elapsed() {
    return std::chrono::duration<double>(std::chrono::system_clock::now() - _start).count();
  }
private:
  std::chrono::time_point<std::chrono::system_clock> _start;
};

}  // namespace help
}  // namespace entityx
