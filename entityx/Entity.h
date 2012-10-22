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


#include <stdint.h>
#include <algorithm>
#include <bitset>
#include <cassert>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>
#include <glog/logging.h>
#include "entityx/Event.h"

namespace entity {

/**
 * Entity handle.
 */
typedef uint64_t Entity;


class EntityManager;


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
 * Components SHOULD provide convenience constructors for initializing on assignment to an Entity.
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
  static Family family() {
    static Family family = family_counter_++;
    // The 64-bit bitmask supports a maximum of 64 components.
    assert(family < 64);
    return family;
  }
};


/**
 * Emitted when an entity is added to the system.
 */
struct EntityCreatedEvent : public Event<EntityCreatedEvent> {
  EntityCreatedEvent(EntityManager &manager, Entity entity) :
      manager(manager), entity(entity) {}

  EntityManager &manager;
  Entity entity;
};


struct EntityDestroyedEvent : public Event<EntityDestroyedEvent> {
  EntityDestroyedEvent(EntityManager &manager, Entity entity) :
      manager(manager), entity(entity) {}

  EntityManager &manager;
  Entity entity;
};


/**
 * Emitted when any component is added to an entity.
 */
template <typename T>
struct ComponentAddedEvent : public Event<ComponentAddedEvent<T>> {
  ComponentAddedEvent(EntityManager &manager, Entity entity, boost::shared_ptr<T> component) :
      manager(manager), entity(entity), component(component) {}

  EntityManager &manager;
  Entity entity;
  boost::shared_ptr<T> component;
};


/**
 * Manages Entity creation and component assignment.
 *
 * eg.
 * EntityManager e;
 *
 * Entity player = e.create();
 *
 * e.assign<Movable>(player);
 * e.assign<Physical>(player);
 * e.assign<Scriptable>(player);
 * shared_ptr<Controllable> controllable = e.assign<Controllable>(player);
 */
class EntityManager : boost::noncopyable {
 private:
  typedef std::vector<boost::shared_ptr<BaseComponent>> EntitiesComponent;

 public:
  EntityManager(EventManager &event_manager) : event_manager_(event_manager) {}

  class View {
   public:
    typedef boost::function<bool (EntityManager &, Entity)> Predicate;

    /// A predicate that excludes entities that don't match the given component mask.
    class ComponentMaskPredicate {
     public:
      ComponentMaskPredicate(const std::vector<uint64_t> &entity_bits, uint64_t mask) : entity_bits_(entity_bits), mask_(mask) {}

      bool operator () (EntityManager &, Entity entity) {
        return (entity_bits_.at(entity) & mask_) == mask_;
      }

     private:
      const std::vector<uint64_t> &entity_bits_;
      uint64_t mask_;
    };

    /// An iterator over a view of the entities in an EntityManager.
    class Iterator : public std::iterator<std::input_iterator_tag, Entity> {
     public:
      Iterator &operator ++ () {
        ++i_;
        next();
        return *this;
      }
      bool operator == (const Iterator& rhs) const { return i_ == rhs.i_; }
      bool operator != (const Iterator& rhs) const { return i_ != rhs.i_; }
      Entity & operator * () { return i_; }
      const Entity & operator * () const { return i_; }

     private:
      friend class View;

      Iterator(EntityManager &manager, const std::vector<Predicate> &predicates,
               const std::vector<boost::function<void (Entity)>> &unpackers, Entity entity)
          : manager_(manager), predicates_(predicates), unpackers_(unpackers), i_(entity) {
        next();
      }

      void next() {
        while (i_ < manager_.size() && !predicate()) {
          ++i_;
        }
        if (i_ < manager_.size()) {
          unpack();
        }
      }

      void unpack() {
        for (auto unpacker : unpackers_) {
          unpacker(i_);
        }
      }

      bool predicate() {
        for (auto &p : predicates_) {
          if (!p(manager_, i_)) {
            return false;
          }
        }
        return true;
      }

      EntityManager &manager_; 
      const std::vector<Predicate> predicates_;
      std::vector<boost::function<void (Entity)>> unpackers_;
      Entity i_;
    };

    // Create a sub-view with an additional predicate.
    View(const View &view, Predicate predicate) : manager_(view.manager_), predicates_(view.predicates_) {
      predicates_.push_back(predicate);
    }

    Iterator begin() { return Iterator(manager_, predicates_, unpackers_, 0); }
    Iterator end() { return Iterator(manager_, predicates_, unpackers_, manager_.size()); }
    const Iterator begin() const { return Iterator(manager_, predicates_, unpackers_, 0); }
    const Iterator end() const { return Iterator(manager_, predicates_, unpackers_, manager_.size()); }

    template <typename A>
    View &unpack_to(boost::shared_ptr<A> &a) {
      unpackers_.push_back([&] (Entity id) {
        a = manager_.component<A>(id);
      });
      return *this;
    }

    template <typename A, typename B, typename ... Args>
    View &unpack_to(boost::shared_ptr<A> &a, boost::shared_ptr<B> &b, Args *& ... args) {
      unpack_to<A>(a);
      return unpack_to<B, Args ...>(b, args ...);
    }
   private:
    friend class EntityManager;

    View(EntityManager &manager, Predicate predicate) : manager_(manager) {
      predicates_.push_back(predicate);
    }

    EntityManager &manager_;
    std::vector<Predicate> predicates_;
    std::vector<boost::function<void (Entity)>> unpackers_;
  };

  /**
   * Number of managed entities.
   */
  size_t size() const { return entity_component_mask_.size(); }

  /**
   * Create a new Entity.
   *
   * Emits EntityCreatedEvent.
   */
  Entity create() {
    Entity id;
    if (free_list_.empty()) {
      id = id_counter_++;
      accomodate_entity(id);
    } else {
      id = *free_list_.erase(free_list_.begin());
    }
    event_manager_.emit<EntityCreatedEvent>(*this, id);
    return id;
  }

  /**
   * Destroy an existing Entity and its associated Components.
   *
   * Emits EntityDestroyedEvent.
   */
  void destroy(Entity entity) {
    CHECK(entity < entity_component_mask_.size()) << "Entity ID outside entity vector range";
    event_manager_.emit<EntityDestroyedEvent>(*this, entity);
    for (auto &components : entity_components_) {
      components.at(entity).reset();
    }
    entity_component_mask_.at(entity) = 0;
    free_list_.insert(entity);
  }

  /**
   * Check if an Entity is registered.
   */
  bool exists(Entity entity) {
    if (entity_component_mask_.empty() || entity >= id_counter_) {
      return false;
    }
    return free_list_.find(entity) == free_list_.end();
  }

  /**
   * Assigns a previously constructed Component to an Entity.
   *
   * @returns component
   */
  template <typename C>
  boost::shared_ptr<C> assign(Entity entity, boost::shared_ptr<C> component) {
    boost::shared_ptr<BaseComponent> base = boost::static_pointer_cast<BaseComponent>(component);
    accomodate_component(C::family());
    entity_components_.at(C::family()).at(entity) = base;
    entity_component_mask_.at(entity) |= uint64_t(1) << C::family();

    event_manager_.emit<ComponentAddedEvent<C>>(*this, entity, component);
    return component;
  }

  /**
   * Assign a Component to an Entity, optionally passing through Component constructor arguments.
   *
   *   shared_ptr<Position> position = em.assign<Position>(e, x, y);
   *
   * @returns Newly created component.
   */
  template <typename C, typename ... Args>
  boost::shared_ptr<C> assign(Entity entity, Args && ... args) {
    return assign<C>(entity, boost::make_shared<C>(args ...));
  }

  /**
   * Retrieve a Component assigned to an Entity.
   *
   * @returns Component instance, or empty shared_ptr<> if the Entity does not have that Component.
   */
  template <typename C>
  boost::shared_ptr<C> component(Entity id) {
    // We don't bother checking the component mask, as we return a nullptr anyway.
    if (C::family() >= entity_components_.size()) {
      return boost::shared_ptr<C>();
    }
    boost::shared_ptr<BaseComponent> c = entity_components_.at(C::family()).at(id);
    return boost::static_pointer_cast<C>(c);
  }

  /**
   * Get all entities with the given component.
   */
  template <typename C>
  View entities_with_components() {
    auto mask = component_mask<C>();
    return View(*this, View::ComponentMaskPredicate(entity_component_mask_, mask));
  }

  /**
   * Find Entities that have all of the specified Components.
   */
  template <typename C1, typename C2, typename ... Components>
  View entities_with_components() {
    auto mask = component_mask<C1, C2, Components ...>();
    return View(*this, View::ComponentMaskPredicate(entity_component_mask_, mask));
  }

  /**
   * Get all entities with the given component.
   */
  template <typename C>
  View entities_with_components(boost::shared_ptr<C> &c) {
    auto mask = component_mask<C>();
    return
        View(*this, View::ComponentMaskPredicate(entity_component_mask_, mask))
        .unpack_to<C>(c);
  }

  /**
   * Find Entities that have all of the specified Components.
   */
  template <typename C1, typename C2, typename ... Components>
  View entities_with_components(boost::shared_ptr<C1> &c1, boost::shared_ptr<C2> &c2, Components *& ... args) {
    auto mask = component_mask<C1, C2, Components ...>();
    return
        View(*this, View::ComponentMaskPredicate(entity_component_mask_, mask))
        .unpack_to<C1, C2, Components...>(c1, c2, args...);
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
  void unpack(Entity id, boost::shared_ptr<A> &a) {
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
  void unpack(Entity id, boost::shared_ptr<A> &a, boost::shared_ptr<B> &b, Args & ... args) {
    unpack<A>(id, a);
    unpack<B, Args ...>(id, b, args ...);
  }

 private:
  template <typename C>
  uint64_t component_mask() {
    return uint64_t(1) << C::family();
  }

  template <typename C1, typename C2, typename ... Components>
  uint64_t component_mask() {
    return component_mask<C1>() | component_mask<C2, Components ...>();
  }

  inline void accomodate_entity(Entity entity) {
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

  Entity id_counter_ = 0;

  EventManager &event_manager_;
  // A nested array of: components = entity_components_[family][entity]
  std::vector<EntitiesComponent> entity_components_;
  // Bitmask of components associated with each entity. Index into the vector is the Entity.
  std::vector<uint64_t> entity_component_mask_;
  // List of available Entity IDs.
  boost::unordered_set<Entity> free_list_;
};

}
