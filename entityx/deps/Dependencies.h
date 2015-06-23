/*
 * Copyright (C) 2013 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */
#pragma once

#include "entityx/System.h"
#include "entityx/Event.h"
#include "entityx/Entity.h"

namespace entityx {
namespace deps {

/**
 * An entityx::System for declaring component dependencies.
 *
 * eg. To declare that a `Physics` component must always be paired with `Position`
 * and `Direction` components:
 *
 *     system_manager->add<Dependency<Physics, Position, Direction>>();
 */
template <typename C, typename ... Deps>
class Dependency : public System<Dependency<C, Deps...>>, public Receiver<Dependency<C, Deps...>> {
public:
  void receive(const ComponentAddedEvent<C> &event) {
    assign<Deps...>(event.entity);
  }

  void configure(EventManager &events) override {
    events.subscribe<ComponentAddedEvent<C>>(*this);
  }

  void update(EntityManager &entities, EventManager &events, TimeDelta dt) override {}

private:
  template <typename D>
  void assign(Entity entity) {
    if (!entity.component<D>()) entity.assign<D>();
  }

  template <typename D, typename D1, typename ... Ds>
  void assign(Entity entity) {
    assign<D>(entity);
    assign<D1, Ds...>(entity);
  }
};

}  // namespace deps
}  // namespace entityx
