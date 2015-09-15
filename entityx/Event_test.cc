/*
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#define CATCH_CONFIG_MAIN

#include <string>
#include <vector>
#include "entityx/3rdparty/catch.hpp"
#include "entityx/Event.h"


using entityx::EventManager;
using entityx::Event;
using entityx::Receiver;


struct Explosion {
  explicit Explosion(int damage) : damage(damage) {}
  int damage;
};


struct Collision {
  explicit Collision(int damage) : damage(damage) {}
  int damage;
};

struct ExplosionSystem : public Receiver<ExplosionSystem> {
  void receive(const Explosion &explosion) {
    damage_received += explosion.damage;
    received_count++;
  }

  void receive(const Collision &collision) {
    damage_received += collision.damage;
    received_count++;
  }

  int received_count = 0;
  int damage_received = 0;
};

TEST_CASE("TestEmitReceive") {
  EventManager em;
  ExplosionSystem explosion_system;
  em.subscribe<Explosion>(explosion_system);
  em.subscribe<Collision>(explosion_system);
  REQUIRE(0 == explosion_system.damage_received);
  em.emit<Explosion>(10);
  REQUIRE(1 == explosion_system.received_count);
  REQUIRE(10 == explosion_system.damage_received);
  em.emit<Collision>(10);
  REQUIRE(20 == explosion_system.damage_received);
  REQUIRE(2 == explosion_system.received_count);
}

TEST_CASE("TestUntypedEmitReceive") {
  EventManager em;
  ExplosionSystem explosion_system;
  em.subscribe<Explosion>(explosion_system);
  REQUIRE(0 == explosion_system.damage_received);
  Explosion explosion(10);
  em.emit(explosion);
  REQUIRE(1 == explosion_system.received_count);
  REQUIRE(10 == explosion_system.damage_received);
}


TEST_CASE("TestReceiverExpired") {
  EventManager em;
  {
    ExplosionSystem explosion_system;
    em.subscribe<Explosion>(explosion_system);
    em.emit<Explosion>(10);
    REQUIRE(10 == explosion_system.damage_received);
    REQUIRE(1 == explosion_system.connected_signals());
    REQUIRE(1 == em.connected_receivers());
  }
  REQUIRE(0 == em.connected_receivers());
}


TEST_CASE("TestSenderExpired") {
  ExplosionSystem explosion_system;
  {
    EventManager em;
    em.subscribe<Explosion>(explosion_system);
    em.emit<Explosion>(10);
    REQUIRE(10 == explosion_system.damage_received);
    REQUIRE(1 == explosion_system.connected_signals());
    REQUIRE(1 == em.connected_receivers());
  }
  REQUIRE(0 == explosion_system.connected_signals());
}

TEST_CASE("TestUnsubscription") {
  ExplosionSystem explosion_system;
  {
    EventManager em;
    em.subscribe<Explosion>(explosion_system);
    REQUIRE(explosion_system.damage_received == 0);
    em.emit<Explosion>(1);
    REQUIRE(explosion_system.damage_received == 1);
    em.unsubscribe<Explosion>(explosion_system);
    em.emit<Explosion>(1);
    REQUIRE(explosion_system.damage_received == 1);
  }
}
