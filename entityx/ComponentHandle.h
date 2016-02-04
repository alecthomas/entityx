/*
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#pragma once

namespace entityx {

class EntityManager;

/**
 * A ComponentHandle<C> is a wrapper around an instance of a component.
 *
 * It provides safe access to components. The handle will be invalidated under
 * the following conditions:
 *
 * - If a component is removed from its host entity.
 * - If its host entity is destroyed.
 */
template <typename C, typename EM>
class ComponentHandle {
public:
  typedef C ComponentType;

  ComponentHandle() : manager_(nullptr) {}

  bool valid() const;
  operator bool() const;

  C *operator -> ();
  const C *operator -> () const;

  C *get();
  const C *get() const;

  /**
   * Remove the component from its entity and destroy it.
   */
  void remove();

  bool operator == (const ComponentHandle<C> &other) const {
    return manager_ == other.manager_ && id_ == other.id_;
  }

  bool operator != (const ComponentHandle<C> &other) const {
    return !(*this == other);
  }

private:
  friend class EntityManager;

  ComponentHandle(EM *manager, Entity::Id id) :
      manager_(manager), id_(id) {}

  EM *manager_;
  Entity::Id id_;
};

}
