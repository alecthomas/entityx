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

#include "entityx/config.h"

namespace entityx {

class EntityManager;

template <typename C, typename EM = EntityManager>
class ComponentHandle;

/** A convenience handle around an Entity::Id.
 *
 * If an entity is destroyed, any copies will be invalidated. Use valid() to
 * check for validity before using.
 *
 * Create entities with `EntityManager`:
 *
 *     Entity entity = entity_manager->create();
 */
class Entity {
public:
  struct Id {
    Id() : id_(0) {}
    explicit Id(uint64_t id) : id_(id) {}
    Id(uint32_t index, uint32_t version) : id_(uint64_t(index) | uint64_t(version) << 32UL) {}

    uint64_t id() const { return id_; }

    bool operator == (const Id &other) const { return id_ == other.id_; }
    bool operator != (const Id &other) const { return id_ != other.id_; }
    bool operator < (const Id &other) const { return id_ < other.id_; }

    uint32_t index() const { return id_ & 0xffffffffUL; }
    uint32_t version() const { return id_ >> 32; }

  private:
    friend class EntityManager;

    uint64_t id_;
  };


  /**
   * Id of an invalid Entity.
   */
  static const Id INVALID;

  Entity() = default;
  Entity(EntityManager *manager, Entity::Id id) : manager_(manager), id_(id) {}
  Entity(const Entity &other) = default;
  Entity &operator = (const Entity &other) = default;

  /**
   * Check if Entity handle is invalid.
   */
  operator bool() const {
    return valid();
  }

  bool operator == (const Entity &other) const {
    return other.manager_ == manager_ && other.id_ == id_;
  }

  bool operator != (const Entity &other) const {
    return !(other == *this);
  }

  bool operator < (const Entity &other) const {
    return other.id_ < id_;
  }

  /**
   * Is this Entity handle valid?
   *
   * In older versions of EntityX, there were no guarantees around entity
   * validity if a previously allocated entity slot was reassigned. That is no
   * longer the case: if a slot is reassigned, old Entity::Id's will be
   * invalid.
   */
  bool valid() const;

  /**
   * Invalidate Entity handle, disassociating it from an EntityManager and invalidating its ID.
   *
   * Note that this does *not* affect the underlying entity and its
   * components. Use destroy() to destroy the associated Entity and components.
   */
  void invalidate();

  Id id() const { return id_; }

  template <typename C, typename ... Args>
  ComponentHandle<C> assign(Args && ... args);

  template <typename C>
  ComponentHandle<C> assign_from_copy(const C &component);

  template <typename C, typename ... Args>
  ComponentHandle<C> replace(Args && ... args);

  template <typename C>
  void remove();

  template <typename C, typename = typename std::enable_if<!std::is_const<C>::value>::type>
  ComponentHandle<C> component();

  template <typename C, typename = typename std::enable_if<std::is_const<C>::value>::type>
  const ComponentHandle<C, const EntityManager> component() const;

  template <typename ... Components>
  std::tuple<ComponentHandle<Components>...> components();

  template <typename ... Components>
  std::tuple<ComponentHandle<const Components, const EntityManager>...> components() const;

  template <typename C>
  bool has_component() const;

  template <typename A, typename ... Args>
  void unpack(ComponentHandle<A> &a, ComponentHandle<Args> & ... args);

  /**
   * Destroy and invalidate this Entity.
   */
  void destroy();

  std::bitset<entityx::MAX_COMPONENTS> component_mask() const;

 private:
  EntityManager *manager_ = nullptr;
  Entity::Id id_ = INVALID;
};

}
