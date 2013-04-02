/**
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 * 
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "entityx/Event.h"

using namespace entityx;
using namespace boost;

struct Explosion : public Event<Explosion> {
  Explosion(int damage) : damage(damage) {}
  int damage;
};

struct ExplosionSystem : public Receiver<ExplosionSystem> {
  void receive(const Explosion &explosion) {
    damage_received += explosion.damage;
  }

  int damage_received = 0;
};

TEST(EventManagerTest, TestEmitReceive) {
  EventManager em;
  ExplosionSystem explosion_system;
  em.subscribe<Explosion>(explosion_system);
  ASSERT_EQ(0, explosion_system.damage_received);
  em.emit<Explosion>(10);
  ASSERT_EQ(10, explosion_system.damage_received);
}
