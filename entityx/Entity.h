/**
 * Copyright (C) 2012 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#pragma once


#include <algorithm>
#include <bitset>
#include <cassert>
#include <iostream>
#include <iterator>
#include <list>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glog/logging.h>
#include "entityx/Event.h"

namespace entityx {


class EntityManager;


/**
 * A convenience handle around an Entity::Id.
 *
 * NOTE: Because Entity IDs are recycled by the EntityManager, there is no way
 * to determine if an Entity is still associated with the same logical entity.
 *
 * The only way to robustly track an Entity through its lifecycle is to
 * subscribe to EntityDestroyedEvent.
 */
class Entity {
 public:
  typedef uint64_t Id;

  /**
   * Id of an invalid Entity.
   */
  static const Id INVALID = Id(-1);

  Entity() {}

  /**
   * Check if Entity handle is invalid.
   */
  bool operator ! () const {
    return !valid();
  }

  bool operator == (const Entity &other) const {
    return other.manager_ == manager_ && other.id_ == id_;
  }

  bool operator != (const Entity &other) const {
    return other.manager_ != manager_ || other.id_ != id_;
  }

  /**
   * Is this Entity handle valid?
   *
   * *NOTE:* This does *not* imply that the originally associated entity ID is
   * the same, or is valid in any way.
   */
  bool valid() const {
    return manager_ != nullptr && id_ != INVALID;
  }

  /**
   * Invalidate Entity handle, disassociating it from an EntityManager and invalidating its ID.
   *
   * Note that this does *not* affect the underlying entity and its components.
   */
  void invalidate();

  Id id() const { return id_; }
  EntityManager &manager() { return *manager_; }

  template <typename C>
  boost::shared_ptr<C> assign(boost::shared_ptr<C> component);
  template <typename C, typename ... Args>
  boost::shared_ptr<C> assign(Args && ... args);

  template <typename C>
  boost::shared_ptr<C> component();

  template <typename A>
  void unpack(boost::shared_ptr<A> &a);
  template <typename A, typename B, typename ... Args>
  void unpack(boost::shared_ptr<A> &a, boost::shared_ptr<B> &b, Args && ... args);

  /**
   * Destroy and invalidate this Entity.
   */
  void destroy();

 private:
  friend class EntityManager;

  Entity(EntityManager *entities, Entity::Id id) : manager_(entities), id_(id) {}

  EntityManager *manager_ = nullptr;
  Entity::Id id_ = INVALID;
};


inline std::ostream &operator << (std::ostream &out, const Entity &entity) {
  out << "Entity(" << entity.id() << ")";
  return out;
}


/**
 * Base component class, only used for insertion into collections.
 *
 * Family is used for registration.
 */
struct BaseComponent {
 public:
  typedef uint64_t Family;

 protected:
  static Family family_counter_;
};


/**
 * Component implementations should inherit from this.
 *
 * Components MUST provide a no-argument constructor.
 * Components SHOULD provide convenience constructors for initializing on assignment to an Entity::Id.
 *
 * This is a struct to imply that components should be data-only.
 *
 * Usage:
 *
 * struct Position : public Component<Position> {
 *   Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
 *
 *   float x, y;
 * };
 *
 * family() is used for registration.
 */
template <typename Derived>
struct Component : public BaseComponent {
 public:
  /// Used internally for registration.
  static Family family();
};


/**
 * Emitted when an entity is added to the system.
 */
struct EntityCreatedEvent : public Event<EntityCreatedEvent> {
  EntityCreatedEvent(Entity entity) : entity(entity) {}

  Entity entity;
};


struct EntityDestroyedEvent : public Event<EntityDestroyedEvent> {
  EntityDestroyedEvent(Entity entity) : entity(entity) {}

  Entity entity;
};


/**
 * Emitted when any component is added to an entity.
 */
template <typename T>
struct ComponentAddedEvent : public Event<ComponentAddedEvent<T>> {
  ComponentAddedEvent(Entity entity, boost::shared_ptr<T> component) :
      entity(entity), component(component) {}

  Entity entity;
  boost::shared_ptr<T> component;
};


/**
 * Manages Entity::Id creation and component assignment.
 *
 * eg.
 * EntityManager e;
 *
 * Entity player = e.create();
 *
 * player.assign<Movable>();
 * player.assign<Physical>();
 * player.assign<Scriptable>();
 * shared_ptr<Controllable> controllable = player.assign<Controllable>();
 */
class EntityManager : boost::noncopyable {
 public:
  static const int MAX_COMPONENTS = 64;

  typedef std::bitset<MAX_COMPONENTS> ComponentMask;

  EntityManager(EventManager &event_manager) : event_manager_(event_manager) {}

  class View {
   public:
    typedef boost::function<bool (EntityManager &, Entity::Id)> Predicate;

    /// A predicate that excludes entities that don't match the given component mask.
    class ComponentMaskPredicate {
     public:
      ComponentMaskPredicate(const std::vector<ComponentMask> &entity_bits, ComponentMask mask) : entity_bits_(entity_bits), mask_(mask) {}

      bool operator () (EntityManager &, Entity::Id entity) {
        return (entity_bits_.at(entity) & mask_) == mask_;
      }

     private:
      const std::vector<ComponentMask> &entity_bits_;
      ComponentMask mask_;
    };

    /// An iterator over a view of the entities in an EntityManager.
    class Iterator : public std::iterator<std::input_iterator_tag, Entity::Id> {
     public:
      Iterator &operator ++ () {
        ++i_;
        next();
        return *this;
      }
      bool operator == (const Iterator& rhs) const { return i_ == rhs.i_; }
      bool operator != (const Iterator& rhs) const { return i_ != rhs.i_; }
      Entity operator * () { return Entity(manager_, i_); }
      const Entity operator * () const { return Entity(manager_, i_); }

     private:
      friend class View;

      Iterator() : manager_(nullptr) {}

      Iterator(EntityManager *manager, const std::vector<Predicate> &predicates,
               const std::vector<boost::function<void (Entity::Id)>> &unpackers, Entity::Id entity)
          : manager_(manager), predicates_(predicates), unpackers_(unpackers), i_(entity) {
        next();
      }

      void next() {
        while (i_ < manager_->size() && !predicate()) {
          ++i_;
        }
        if (i_ < manager_->size() && !unpackers_.empty()) {
          for (auto unpacker : unpackers_) {
            unpacker(i_);
          }
        }
      }

      bool predicate() {
        for (auto &p : predicates_) {
          if (!p(*manager_, i_)) {
            return false;
          }
        }
        return true;
      }

      EntityManager *manager_;
      const std::vector<Predicate> predicates_;
      std::vector<boost::function<void (Entity::Id)>> unpackers_;
      Entity::Id i_;
    };

    // Create a sub-view with an additional predicate.
    View(const View &view, Predicate predicate) : manager_(view.manager_), predicates_(view.predicates_) {
      predicates_.push_back(predicate);
    }

    Iterator begin() { return Iterator(manager_, predicates_, unpackers_, 0); }
    Iterator end() { return Iterator(manager_, predicates_, unpackers_, manager_->size()); }
    const Iterator begin() const { return Iterator(manager_, predicates_, unpackers_, 0); }
    const Iterator end() const { return Iterator(manager_, predicates_, unpackers_, manager_->size()); }

    template <typename A>
    View &unpack_to(boost::shared_ptr<A> &a) {
      unpackers_.push_back(Unpacker<A>(manager_, a));
      // This resulted in a segfault under clang 4.1 on OSX. No idea why.
      /*unpackers_.push_back([&a, this](Entity::Id id) {
        a = manager_.component<A>(id);
      });*/
      return *this;
    }

    template <typename A, typename B, typename ... Args>
    View &unpack_to(boost::shared_ptr<A> &a, boost::shared_ptr<B> &b, Args && ... args) {
      unpack_to<A>(a);
      return unpack_to<B, Args ...>(b, args ...);
    }
   private:
    friend class EntityManager;

    template <typename T>
    struct Unpacker {
      Unpacker(EntityManager *manager, boost::shared_ptr<T> &c) : manager_(manager), c(c) {}

      void operator () (Entity::Id id) {
        c = manager_->component<T>(id);
      }

     private:
      EntityManager *manager_;
      boost::shared_ptr<T> &c;
    };

    View(EntityManager *manager, Predicate predicate) : manager_(manager) {
      predicates_.push_back(predicate);
    }

    EntityManager *manager_;
    std::vector<Predicate> predicates_;
    std::vector<boost::function<void (Entity::Id)>> unpackers_;
  };

  /**
   * Number of managed entities.
   */
  size_t size() const { return entity_component_mask_.size(); }

  /**
   * Create a new Entity::Id.
   *
   * Emits EntityCreatedEvent.
   */
  Entity create() {
    Entity::Id id;
    if (free_list_.empty()) {
      id = id_counter_++;
      accomodate_entity(id);
    } else {
      id = free_list_.front();
      free_list_.pop_front();
    }
    event_manager_.emit<EntityCreatedEvent>(Entity(this, id));
    return Entity(this, id);
  }

  /**
   * Destroy an existing Entity::Id and its associated Components.
   *
   * Emits EntityDestroyedEvent.
   */
  void destroy(Entity::Id entity) {
    CHECK(entity < entity_component_mask_.size()) << "Entity::Id ID outside entity vector range";
    event_manager_.emit<EntityDestroyedEvent>(Entity(this, entity));
    for (auto &components : entity_components_) {
      components.at(entity).reset();
    }
    entity_component_mask_.at(entity) = 0;
    free_list_.push_back(entity);
  }

  Entity get(Entity::Id id) {
    return Entity(this, id);
  }

  /**
   * Assigns a previously constructed Component to an Entity::Id.
   *
   * @returns component
   */
  template <typename C>
  boost::shared_ptr<C> assign(Entity::Id entity, boost::shared_ptr<C> component) {
    boost::shared_ptr<BaseComponent> base = boost::static_pointer_cast<BaseComponent>(component);
    accomodate_component(C::family());
    entity_components_.at(C::family()).at(entity) = base;
    entity_component_mask_.at(entity) |= uint64_t(1) << C::family();

    event_manager_.emit<ComponentAddedEvent<C>>(Entity(this, entity), component);
    return component;
  }

  /**
   * Assign a Component to an Entity::Id, optionally passing through Component constructor arguments.
   *
   *   shared_ptr<Position> position = em.assign<Position>(e, x, y);
   *
   * @returns Newly created component.
   */
  template <typename C, typename ... Args>
  boost::shared_ptr<C> assign(Entity::Id entity, Args && ... args) {
    return assign<C>(entity, boost::make_shared<C>(args ...));
  }

  /**
   * Retrieve a Component assigned to an Entity::Id.
   *
   * @returns Component instance, or empty shared_ptr<> if the Entity::Id does not have that Component.
   */
  template <typename C>
  boost::shared_ptr<C> component(Entity::Id id) {
    // We don't bother checking the component mask, as we return a nullptr anyway.
    if (C::family() >= entity_components_.size()) {
      return boost::shared_ptr<C>();
    }
    boost::shared_ptr<BaseComponent> c = entity_components_.at(C::family()).at(id);
    return boost::static_pointer_cast<C>(c);
  }

  /**
   * Find Entities that have all of the specified Components.
   */
  template <typename C, typename ... Components>
  View entities_with_components() {
    auto mask = component_mask<C, Components ...>();
    return View(this, View::ComponentMaskPredicate(entity_component_mask_, mask));
  }

  /**
   * Find Entities that have all of the specified Components.
   */
  template <typename C, typename ... Components>
  View entities_with_components(boost::shared_ptr<C> &c, Components && ... args) {
    auto mask = component_mask(c, args ...);
    return
        View(this, View::ComponentMaskPredicate(entity_component_mask_, mask))
        .unpack_to(c, args ...);
  }

  /**
   * Unpack components directly into pointers.
   *
   * Components missing from the entity will be set to nullptr.
   *
   * Useful for fast bulk iterations.
   *
   * boost::shared_ptr<Position> p;
   * boost::shared_ptr<Direction> d;
   * unpack<Position, Direction>(e, p, d);
   */
  template <typename A>
  void unpack(Entity::Id id, boost::shared_ptr<A> &a) {
    a = component<A>(id);
  }

  /**
   * Unpack components directly into pointers.
   *
   * Components missing from the entity will be set to nullptr.
   *
   * Useful for fast bulk iterations.
   *
   * boost::shared_ptr<Position> p;
   * boost::shared_ptr<Direction> d;
   * unpack<Position, Direction>(e, p, d);
   */
  template <typename A, typename B, typename ... Args>
  void unpack(Entity::Id id, boost::shared_ptr<A> &a, boost::shared_ptr<B> &b, Args && ... args) {
    unpack<A>(id, a);
    unpack<B, Args ...>(id, b, args ...);
  }

 private:
  template <typename C>
  ComponentMask component_mask() {
    ComponentMask mask;
    mask.set(C::family());
    return mask;
  }

  template <typename C1, typename C2, typename ... Components>
  ComponentMask component_mask() {
    return component_mask<C1>() | component_mask<C2, Components ...>();
  }

  template <typename C>
  ComponentMask component_mask(const boost::shared_ptr<C> &c) {
    return component_mask<C>();
  }

  template <typename C1, typename C2, typename ... Components>
  ComponentMask component_mask(const boost::shared_ptr<C1> &c1, const boost::shared_ptr<C2> &c2, Components && ... args) {
    return component_mask<C1>(c1) | component_mask<C2, Components ...>(c2, args...);
  }

  inline void accomodate_entity(Entity::Id entity) {
    if (entity_component_mask_.size() <= entity) {
      entity_component_mask_.resize(entity + 1);
      for (auto &components : entity_components_) {
          components.resize(entity + 1);
      }
    }
  }

  inline void accomodate_component(BaseComponent::Family family) {
    if (entity_components_.size() <= family) {
      entity_components_.resize(family + 1);
      for (auto &components : entity_components_) {
          components.resize(id_counter_);
      }
    }
  }

  Entity::Id id_counter_ = 0;

  EventManager &event_manager_;
  // A nested array of: components = entity_components_[family][entity]
  std::vector<std::vector<boost::shared_ptr<BaseComponent>>> entity_components_;
  // Bitmask of components associated with each entity. Index into the vector is the Entity::Id.
  std::vector<ComponentMask> entity_component_mask_;
  // List of available Entity::Id IDs.
  std::list<Entity::Id> free_list_;
};

template <typename C>
BaseComponent::Family Component<C>::family() {
  static Family family = family_counter_++;
  assert(family < EntityManager::MAX_COMPONENTS);
  return family;
}

template <typename C>
boost::shared_ptr<C> Entity::assign(boost::shared_ptr<C> component) {
  return manager_->assign<C>(id_, component);
}

template <typename C, typename ... Args>
boost::shared_ptr<C> Entity::assign(Args && ... args) {
  return manager_->assign<C>(id_, args ...);
}

template <typename C>
boost::shared_ptr<C> Entity::component() {
  return manager_->component<C>(id_);
}

template <typename A>
void Entity::unpack(boost::shared_ptr<A> &a) {
  manager_->unpack(id_, a);
}

template <typename A, typename B, typename ... Args>
void Entity::unpack(boost::shared_ptr<A> &a, boost::shared_ptr<B> &b, Args && ... args) {
  manager_->unpack(id_, a, b, args ...);
}

}
