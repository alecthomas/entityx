/*
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "entityx/Event.h"


using entityx::EventManager;
using entityx::Event;
using entityx::Receiver;


struct Explosion : public Event<Explosion> {
  explicit Explosion(int damage) : damage(damage) {}
  int damage;
};

struct ExplosionSystem : public Receiver<ExplosionSystem> {
  void receive(const Explosion &explosion) {
    damage_received += explosion.damage;
  }

  int damage_received = 0;
};

TEST(EventManagerTest, TestEmitReceive) {
  auto em = EventManager::make();
  ExplosionSystem explosion_system;
  em->subscribe<Explosion>(explosion_system);
  ASSERT_EQ(0, explosion_system.damage_received);
  em->emit<Explosion>(10);
  ASSERT_EQ(10, explosion_system.damage_received);
}


TEST(EventManagerTest, TestReceiverExpired) {
  auto em = EventManager::make();
  {
    ExplosionSystem explosion_system;
    em->subscribe<Explosion>(explosion_system);
    em->emit<Explosion>(10);
    ASSERT_EQ(10, explosion_system.damage_received);
    ASSERT_EQ(1, explosion_system.connected_signals());
    ASSERT_EQ(1, em->connected_receivers());
  }
  ASSERT_EQ(0, em->connected_receivers());
}


TEST(EventManagerTest, TestSenderExpired) {
  ExplosionSystem explosion_system;
  {
    auto em = EventManager::make();
    em->subscribe<Explosion>(explosion_system);
    em->emit<Explosion>(10);
    ASSERT_EQ(10, explosion_system.damage_received);
    ASSERT_EQ(1, explosion_system.connected_signals());
    ASSERT_EQ(1, em->connected_receivers());
  }
  ASSERT_EQ(0, explosion_system.connected_signals());
}
