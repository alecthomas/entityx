/*
 * Copyright (C) 2012-2014 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include "entityx/help/Pool.h"

namespace entityx {

BasePool::~BasePool() {
  for (char *ptr : blocks_) {
    delete[] ptr;
  }
}

}  // namespace entityx
