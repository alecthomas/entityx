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


#include <stdint.h>
#include <algorithm>
#include <bitset>
#include <cassert>
#include <iostream>
#include <iterator>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "entityx/config.h"
#include "entityx/Event.h"
#include "entityx/help/NonCopyable.h"

namespace entityx {


class EntityManager;


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

  Entity() {}
  Entity(const Entity &) = default;
  Entity(Entity &&) = default;
  Entity &operator = (const Entity &) = default;

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

  template <typename C>
  ptr<C> assign(ptr<C> component);
  template <typename C, typename ... Args>
  ptr<C> assign(Args && ... args);

  template <typename C>
  ptr<C> remove();

  template <typename C>
  ptr<C> component();

  template <typename A>
  void unpack(ptr<A> &a);
  template <typename A, typename B, typename ... Args>
  void unpack(ptr<A> &a, ptr<B> &b, Args && ... args);

  /**
   * Destroy and invalidate this Entity.
   */
  void destroy();

 private:
  friend class EntityManager;

  Entity(ptr<EntityManager> manager, Entity::Id id) : manager_(manager), id_(id) {}

  weak_ptr<EntityManager> manager_;
  Entity::Id id_ = INVALID;
};


inline std::ostream &operator << (std::ostream &out, const Entity::Id &id) {
  out << "Entity::Id(" << std::hex << id.id() << ")";
  return out;
}


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
 *     struct Position : public Component<Position> {
 *       Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
 *
 *       float x, y;
 *     };
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
  explicit EntityCreatedEvent(Entity entity) : entity(entity) {}

  Entity entity;
};


/**
 * Called just prior to an entity being destroyed.
 */
struct EntityDestroyedEvent : public Event<EntityDestroyedEvent> {
  explicit EntityDestroyedEvent(Entity entity) : entity(entity) {}

  Entity entity;
};


/**
 * Emitted when any component is added to an entity.
 */
template <typename T>
struct ComponentAddedEvent : public Event<ComponentAddedEvent<T>> {
  ComponentAddedEvent(Entity entity, ptr<T> component) :
      entity(entity), component(component) {}

  Entity entity;
  ptr<T> component;
};

/**
 * Emitted when any component is removed from an entity.
 */
template <typename T>
struct ComponentRemovedEvent : public Event<ComponentRemovedEvent<T>> {
  ComponentRemovedEvent(Entity entity, ptr<T> component) :
      entity(entity), component(component) {}

  Entity entity;
  ptr<T> component;
};

/**
 * Manages Entity::Id creation and component assignment.
 */
class EntityManager : entityx::help::NonCopyable, public enable_shared_from_this<EntityManager> {
 public:
  typedef std::bitset<entityx::MAX_COMPONENTS> ComponentMask;

  explicit EntityManager(ptr<EventManager> event_manager);
  virtual ~EntityManager();

  static ptr<EntityManager> make(ptr<EventManager> event_manager) {
    return ptr<EntityManager>(new EntityManager(event_manager));
  }

  class View {
   public:
    typedef std::function<bool (const ptr<EntityManager> &, const Entity::Id &)> Predicate;

    /// A predicate that matches valid entities with the given component mask.
    class ComponentMaskPredicate {
     public:
      ComponentMaskPredicate(const std::vector<ComponentMask> &entity_components, ComponentMask mask)
          : entity_components_(entity_components), mask_(mask) {}

      bool operator()(const ptr<EntityManager> &entities, const Entity::Id &entity) {
        return entities->entity_version_[entity.index()] == entity.version()
            && (entity_components_[entity.index()] & mask_) == mask_;
      }

     private:
      const std::vector<ComponentMask> &entity_components_;
      ComponentMask mask_;
    };

    struct BaseUnpacker {
      virtual ~BaseUnpacker() {}
      virtual void unpack(const Entity::Id &id) = 0;
    };

    /// An iterator over a view of the entities in an EntityManager.
    class Iterator : public std::iterator<std::input_iterator_tag, Entity::Id> {
     public:
      Iterator &operator ++() {
        ++i_;
        next();
        return *this;
      }
      bool operator == (const Iterator& rhs) const { return i_ == rhs.i_; }
      bool operator != (const Iterator& rhs) const { return i_ != rhs.i_; }
      Entity operator * () { return Entity(manager_, manager_->create_id(i_)); }
      const Entity operator * () const { return Entity(manager_, manager_->create_id(i_)); }

     private:
      friend class View;

      Iterator(ptr<EntityManager> manager,
               const std::vector<Predicate> &predicates,
               const std::vector<ptr<BaseUnpacker>> &unpackers,
               uint32_t index)
          : manager_(manager), predicates_(predicates), unpackers_(unpackers), i_(index), capacity_(manager_->capacity()) {
        next();
      }

      void next() {
        while (i_ < capacity_ && !predicate()) {
          ++i_;
        }

        if (i_ < capacity_ && !unpackers_.empty()) {
          Entity::Id id = manager_->create_id(i_);
          for (auto &unpacker : unpackers_) {
            unpacker->unpack(id);
          }
        }
      }

      bool predicate() {
        Entity::Id id = manager_->create_id(i_);
        for (auto &p : predicates_) {
          if (!p(manager_, id)) {
            return false;
          }
        }
        return true;
      }

      ptr<EntityManager> manager_;
      std::vector<Predicate> predicates_;
      std::vector<ptr<BaseUnpacker>> unpackers_;
      uint32_t i_;
      size_t capacity_;
    };

    // Create a sub-view with an additional predicate.
    View(const View &view, Predicate predicate) : manager_(view.manager_), predicates_(view.predicates_) {
      predicates_.push_back(predicate);
    }

    Iterator begin() { return Iterator(manager_, predicates_, unpackers_, 0); }
    Iterator end() { return Iterator(manager_, predicates_, unpackers_, manager_->capacity()); }
    const Iterator begin() const { return Iterator(manager_, predicates_, unpackers_, 0); }
    const Iterator end() const { return Iterator(manager_, predicates_, unpackers_, manager_->capacity()); }

    template <typename A>
    View &unpack_to(ptr<A> &a) {
      unpackers_.push_back(ptr<Unpacker<A>>(new Unpacker<A>(manager_, a)));
      return *this;
    }

    template <typename A, typename B, typename ... Args>
    View &unpack_to(ptr<A> &a, ptr<B> &b, Args && ... args) {
      unpack_to<A>(a);
      return unpack_to<B, Args ...>(b, args ...);
    }

   private:
    friend class EntityManager;

    template <typename T>
    struct Unpacker : BaseUnpacker {
      Unpacker(ptr<EntityManager> manager, ptr<T> &c) : manager_(manager), c(c) {}

      void unpack(const Entity::Id &id) {
        c = manager_->component<T>(id);
      }

     private:
      ptr<EntityManager> manager_;
      ptr<T> &c;
    };

    View(ptr<EntityManager> manager, Predicate predicate) : manager_(manager) {
      predicates_.push_back(predicate);
    }

    ptr<EntityManager> manager_;
    std::vector<Predicate> predicates_;
    std::vector<ptr<BaseUnpacker>> unpackers_;
  };

  /**
   * Number of managed entities.
   */
  size_t size() const { return entity_component_mask_.size() - free_list_.size(); }

  /**
   * Current entity capacity.
   */
  size_t capacity() const { return entity_component_mask_.size(); }

  /**
   * Return true if the given entity ID is still valid.
   */
  bool valid(Entity::Id id) {
    return id.index() < entity_version_.size() && entity_version_[id.index()] == id.version();
  }

  /**
   * Create a new Entity::Id.
   *
   * Emits EntityCreatedEvent.
   */
  Entity create() {
    uint32_t index, version;
    if (free_list_.empty()) {
      index = index_counter_++;
      accomodate_entity(index);
      version = entity_version_[index] = 1;
    } else {
      index = free_list_.front();
      free_list_.pop_front();
       version = entity_version_[index];
    }
    Entity entity(shared_from_this(), Entity::Id(index, version));
    event_manager_->emit<EntityCreatedEvent>(entity);
    return entity;
  }

  /**
   * Destroy an existing Entity::Id and its associated Components.
   *
   * Emits EntityDestroyedEvent.
   */
  void destroy(Entity::Id entity) {
    assert(entity.index() < entity_component_mask_.size() && "Entity::Id ID outside entity vector range");
    assert(entity_version_[entity.index()] == entity.version() && "Attempt to destroy Entity using a stale Entity::Id");
    event_manager_->emit<EntityDestroyedEvent>(Entity(shared_from_this(), entity));
    for (auto &components : entity_components_) {
      components[entity.index()].reset();
    }
    entity_component_mask_[entity.index()] = 0;
    entity_version_[entity.index()]++;
    free_list_.push_back(entity.index());
  }

  Entity get(Entity::Id id) {
    assert(entity_version_[id.index()] == id.version() && "Attempt to get() with stale Entity::Id");
    return Entity(shared_from_this(), id);
  }

  /**
   * Create an Entity::Id for a slot.
   *
   * NOTE: Does *not* check for validity, but the Entity::Id constructor will
   * fail if the ID is invalid.
   */
  Entity::Id create_id(uint32_t index) const {
    return Entity::Id(index, entity_version_[index]);
  }

  /**
   * Assigns a previously constructed Component to an Entity::Id.
   *
   * @returns component
   */
  template <typename C>
  ptr<C> assign(Entity::Id id, ptr<C> component) {
    ptr<BaseComponent> base(static_pointer_cast<BaseComponent>(component));
    accomodate_component(C::family());
    entity_components_[C::family()][id.index()] = base;
    entity_component_mask_[id.index()] |= uint64_t(1) << C::family();

    event_manager_->emit<ComponentAddedEvent<C>>(Entity(shared_from_this(), id), component);
    return component;
  }

  /**
   * Assign a Component to an Entity::Id, optionally passing through Component constructor arguments.
   *
   *     ptr<Position> position = em.assign<Position>(e, x, y);
   *
   * @returns Newly created component.
   */
  template <typename C, typename ... Args>
  ptr<C> assign(Entity::Id entity, Args && ... args) {
    return assign<C>(entity, ptr<C>(new C(args ...)));
  }

  /**
   * Remove a Component from an Entity::Id
   *
   * Emits a ComponentRemovedEvent<C> event.
   */
  template <typename C>
  ptr<C> remove(const Entity::Id &id) {
    ptr<C> component(static_pointer_cast<C>(entity_components_[C::family()][id.index()]));
    entity_components_[C::family()][id.index()].reset();
    entity_component_mask_[id.index()] &= ~(uint64_t(1) << C::family());
    if (component)
      event_manager_->emit<ComponentRemovedEvent<C>>(Entity(shared_from_this(), id), component);
    return component;
  }

  /**
   * Retrieve a Component assigned to an Entity::Id.
   *
   * @returns Component instance, or empty ptr<> if the Entity::Id does not have that Component.
   */
  template <typename C>
  ptr<C> component(const Entity::Id &id) {
    // We don't bother checking the component mask, as we return a nullptr anyway.
    if (C::family() >= entity_components_.size()) {
      return ptr<C>();
    }
    ptr<BaseComponent> c = entity_components_[C::family()][id.index()];
    return ptr<C>(static_pointer_cast<C>(c));
  }

  /**
   * Find Entities that have all of the specified Components.
   */
  template <typename C, typename ... Components>
  View entities_with_components() {
    auto mask = component_mask<C, Components ...>();
    return View(shared_from_this(), View::ComponentMaskPredicate(entity_component_mask_, mask));
  }

  /**
   * Find Entities that have all of the specified Components.
   */
  template <typename C, typename ... Components>
  View entities_with_components(ptr<C> &c, Components && ... args) {
    auto mask = component_mask(c, args ...);
    return
        View(shared_from_this(), View::ComponentMaskPredicate(entity_component_mask_, mask))
        .unpack_to(c, args ...);
  }

  /**
   * Unpack components directly into pointers.
   *
   * Components missing from the entity will be set to nullptr.
   *
   * Useful for fast bulk iterations.
   *
   * ptr<Position> p;
   * ptr<Direction> d;
   * unpack<Position, Direction>(e, p, d);
   */
  template <typename A>
  void unpack(Entity::Id id, ptr<A> &a) {
    a = component<A>(id);
  }

  /**
   * Unpack components directly into pointers.
   *
   * Components missing from the entity will be set to nullptr.
   *
   * Useful for fast bulk iterations.
   *
   * ptr<Position> p;
   * ptr<Direction> d;
   * unpack<Position, Direction>(e, p, d);
   */
  template <typename A, typename B, typename ... Args>
  void unpack(Entity::Id id, ptr<A> &a, ptr<B> &b, Args && ... args) {
    unpack<A>(id, a);
    unpack<B, Args ...>(id, b, args ...);
  }

  /**
   * Destroy all entities from this EntityManager.
   */
  void destroy_all();

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
  ComponentMask component_mask(const ptr<C> &c) {
    return component_mask<C>();
  }

  template <typename C1, typename C2, typename ... Components>
  ComponentMask component_mask(const ptr<C1> &c1, const ptr<C2> &c2, Components && ... args) {
    return component_mask<C1>(c1) | component_mask<C2, Components ...>(c2, args...);
  }

  inline void accomodate_entity(uint32_t index) {
    if (entity_component_mask_.size() <= index) {
      entity_component_mask_.resize(index + 1);
      entity_version_.resize(index + 1);
      for (auto &components : entity_components_) {
          components.resize(index + 1);
      }
    }
  }

  inline void accomodate_component(BaseComponent::Family family) {
    if (entity_components_.size() <= family) {
      entity_components_.resize(family + 1);
      for (auto &components : entity_components_) {
          components.resize(index_counter_);
      }
    }
  }

  uint32_t index_counter_ = 0;

  ptr<EventManager> event_manager_;
  // A nested array of: components = entity_components_[family][entity]
  std::vector<std::vector<ptr<BaseComponent>>> entity_components_;
  // Bitmask of components associated with each entity. Index into the vector is the Entity::Id.
  std::vector<ComponentMask> entity_component_mask_;
  // Vector of entity version numbers. Incremented each time an entity is destroyed
  std::vector<uint32_t> entity_version_;
  // List of available entity slots.
  std::list<uint32_t> free_list_;
};

template <typename C>
BaseComponent::Family Component<C>::family() {
  static Family family = family_counter_++;
  assert(family < entityx::MAX_COMPONENTS);
  return family;
}

template <typename C>
ptr<C> Entity::assign(ptr<C> component) {
  assert(valid());
  return manager_.lock()->assign<C>(id_, component);
}

template <typename C, typename ... Args>
ptr<C> Entity::assign(Args && ... args) {
  assert(valid());
  return manager_.lock()->assign<C>(id_, args ...);
}

template <typename C>
ptr<C> Entity::remove() {
  assert(valid() && component<C>());
  return manager_.lock()->remove<C>(id_);
}

template <typename C>
ptr<C> Entity::component() {
  assert(valid());
  return manager_.lock()->component<C>(id_);
}

template <typename A>
void Entity::unpack(ptr<A> &a) {
  assert(valid());
  manager_.lock()->unpack(id_, a);
}

template <typename A, typename B, typename ... Args>
void Entity::unpack(ptr<A> &a, ptr<B> &b, Args && ... args) {
  assert(valid());
  manager_.lock()->unpack(id_, a, b, args ...);
}

}  // namespace entityx
