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
#include <new>
#include <cstdlib>
#include <algorithm>
#include <bitset>
#include <cassert>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "entityx/help/Pool.h"
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
  Entity(EntityManager *manager, Entity::Id id) : manager_(manager), id_(id) {}
  Entity(const Entity &other) : manager_(other.manager_), id_(other.id_) {}

  Entity &operator = (const Entity &other) {
    manager_ = other.manager_;
    id_ = other.id_;
    return *this;
  }

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

  template <typename C, typename ... Args>
  C *assign(Args && ... args);

  template <typename C>
  void remove();

  template <typename C>
  C *component();

  template <typename A, typename ... Args>
  void unpack(A *&a, Args *& ... args);

  /**
   * Destroy and invalidate this Entity.
   */
  void destroy();

  std::bitset<entityx::MAX_COMPONENTS> component_mask() const;

 private:
  EntityManager *manager_ = nullptr;
  Entity::Id id_ = INVALID;
};


inline std::ostream &operator << (std::ostream &out, const Entity::Id &id) {
  out << "Entity::Id(" << id.index() << "." << id.version() << ")";
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

  // NOTE: Component memory is *always* managed by the EntityManager.
  // Use Entity::destroy() instead.
  void operator delete(void *p) { throw std::bad_alloc(); }
  void operator delete[](void *p) { throw std::bad_alloc(); }


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
template <typename C>
struct ComponentAddedEvent : public Event<ComponentAddedEvent<C>> {
  ComponentAddedEvent(Entity entity, C *component) :
      entity(entity), component(component) {}

  Entity entity;
  C *component;
};

/**
 * Emitted when any component is removed from an entity.
 */
template <typename C>
struct ComponentRemovedEvent : public Event<ComponentRemovedEvent<C>> {
  ComponentRemovedEvent(Entity entity, C *component) :
      entity(entity), component(component) {}

  Entity entity;
  C *component;
};


/**
 * Manages Entity::Id creation and component assignment.
 */
class EntityManager : entityx::help::NonCopyable {
 public:
  typedef std::bitset<entityx::MAX_COMPONENTS> ComponentMask;

  explicit EntityManager(EventManager &event_manager);
  virtual ~EntityManager();

  class View {
   public:
    typedef std::function<bool (const EntityManager &, const Entity::Id &)> Predicate;

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

      Iterator(EntityManager *manager,
               const std::vector<Predicate> &predicates,
               const std::vector<std::shared_ptr<BaseUnpacker>> &unpackers,
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
          if (!p(*manager_, id)) {
            return false;
          }
        }
        return true;
      }

      EntityManager *manager_;
      std::vector<Predicate> predicates_;
      std::vector<std::shared_ptr<BaseUnpacker>> unpackers_;
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
    View &unpack_to(A *&a) {
      unpackers_.push_back(std::shared_ptr<Unpacker<A>>(new Unpacker<A>(manager_, a)));
      return *this;
    }

    template <typename A, typename B, typename ... Args>
    View &unpack_to(A *&a, B *&b, Args *& ... args) {
      unpack_to<A>(a);
      return unpack_to<B, Args ...>(b, args ...);
    }

   private:
    friend class EntityManager;

    template <typename C>
    struct Unpacker : BaseUnpacker {
      Unpacker(EntityManager *manager, C *&c) : manager_(manager), c(c) {}

      void unpack(const Entity::Id &id) {
        c = manager_->component<C>(id);
      }

     private:
      EntityManager *manager_;
      C *&c;
    };

    View(EntityManager *manager, Predicate predicate) : manager_(manager) {
      predicates_.push_back(predicate);
    }

    EntityManager *manager_;
    std::vector<Predicate> predicates_;
    std::vector<std::shared_ptr<BaseUnpacker>> unpackers_;
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
    Entity entity(this, Entity::Id(index, version));
    event_manager_.emit<EntityCreatedEvent>(entity);
    return entity;
  }

  /**
   * Destroy an existing Entity::Id and its associated Components.
   *
   * Emits EntityDestroyedEvent.
   */
  void destroy(Entity::Id entity) {
    assert_valid(entity);
    int index = entity.index();
    event_manager_.emit<EntityDestroyedEvent>(Entity(this, entity));
    for (BasePool *pool : component_pools_) {
      if (pool) pool->destroy(index);
    }
    entity_component_mask_[index] = 0;
    entity_version_[index]++;
    free_list_.push_back(index);
  }

  Entity get(Entity::Id id) {
    assert_valid(id);
    return Entity(this, id);
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
   * Assign a Component to an Entity::Id, passing through Component constructor arguments.
   *
   *     Position &position = em.assign<Position>(e, x, y);
   *
   * @returns Smart pointer to newly created component.
   */
  template <typename C, typename ... Args>
  C *assign(Entity::Id id, Args && ... args) {
    assert_valid(id);
    const int family = C::family();
    // Placement new into the component pool.
    Pool<C> *pool = accomodate_component<C>();
    C *component = new(pool->get(id.index())) C(std::forward<Args>(args) ...);
    entity_component_mask_[id.index()] |= uint64_t(1) << family;
    event_manager_.emit<ComponentAddedEvent<C>>(Entity(this, id), component);
    return component;
  }

  /**
   * Remove a Component from an Entity::Id
   *
   * Emits a ComponentRemovedEvent<C> event.
   */
  template <typename C>
  void remove(Entity::Id id) {
    assert_valid(id);
    const int family = C::family();
    const int index = id.index();
    BasePool *pool = component_pools_[family];
    C *component = static_cast<C*>(pool->get(id.index()));
    entity_component_mask_[id.index()] &= ~(uint64_t(1) << family);
    event_manager_.emit<ComponentRemovedEvent<C>>(Entity(this, id), component);
    pool->destroy(index);
  }

  /**
   * Retrieve a Component assigned to an Entity::Id.
   *
   * @returns Pointer to an instance of C, or nullptr if the Entity::Id does not have that Component.
   */
  template <typename C>
  C *component(Entity::Id id) {
    assert_valid(id);
    size_t family = C::family();
    // We don't bother checking the component mask, as we return a nullptr anyway.
    if (family >= component_pools_.size())
      return nullptr;
    BasePool *pool = component_pools_[family];
    if (!pool || !entity_component_mask_[id.index()][family])
      return nullptr;
    return static_cast<C*>(pool->get(id.index()));
  }

  /**
   * Retrieve a Component assigned to an Entity::Id.
   *
   * @returns Component instance, or nullptr if the Entity::Id does not have that Component.
   */
  template <typename C>
  const C *component(Entity::Id id) const {
    assert_valid(id);
    size_t family = C::family();
    // We don't bother checking the component mask, as we return a nullptr anyway.
    if (family >= component_pools_.size())
      return nullptr;
    BasePool *pool = component_pools_[family];
    if (!pool || !entity_component_mask_[id.index()][family])
      return nullptr;
    return static_cast<const C*>(pool->get(id.index()));
  }

  /**
   * Find Entities that have all of the specified Components.
   *
   * @code
   * for (Entity entity : entity_manager.entities_with_components<Position, Direction>()) {
   *   Position *position = entity.component<Position>();
   *   Direction *direction = entity.component<Direction>();
   *
   *   ...
   * }
   * @endcode
   */
  template <typename C, typename ... Components>
  View entities_with_components() {
    auto mask = component_mask<C, Components ...>();
    return View(this, ComponentMaskPredicate(entity_component_mask_, mask));
  }

  /**
   * Find Entities that have all of the specified Components and assign them
   * to the given parameters.
   *
   * @code
   * Position *position;
   * Direction *direction;
   * for (Entity entity : entity_manager.entities_with_components(position, direction)) {
   *   // Use position and component here.
   * }
   * @endcode
   */
  template <typename C, typename ... Components>
  View entities_with_components(C *&c, Components *& ... args) {
    auto mask = component_mask(c, args ...);
    return
        View(this, ComponentMaskPredicate(entity_component_mask_, mask))
        .unpack_to(c, args ...);
  }

  template <typename A>
  void unpack(Entity::Id id, A *&a) {
    assert_valid(id);
    a = component<A>(id);
  }

  /**
   * Unpack components directly into pointers.
   *
   * Components missing from the entity will be set to nullptr.
   *
   * Useful for fast bulk iterations.
   *
   * ComponentPtr<Position> p;
   * ComponentPtr<Direction> d;
   * unpack<Position, Direction>(e, p, d);
   */
  template <typename A, typename ... Args>
  void unpack(Entity::Id id, A *&a, Args *& ... args) {
    assert_valid(id);
    a = component<A>(id);
    unpack<Args ...>(id, args ...);
  }

  /**
   * Destroy all entities and reset the EntityManager.
   */
  void reset();

 private:
  friend class Entity;

  /// A predicate that matches valid entities with the given component mask.
  class ComponentMaskPredicate {
   public:
    ComponentMaskPredicate(const std::vector<ComponentMask> &entity_component_masks, ComponentMask mask)
        : entity_component_masks_(entity_component_masks), mask_(mask) {}

    bool operator()(const EntityManager &entities, const Entity::Id &entity) {
      return entities.entity_version_[entity.index()] == entity.version()
          && (entity_component_masks_[entity.index()] & mask_) == mask_;
    }

   private:
    const std::vector<ComponentMask> &entity_component_masks_;
    ComponentMask mask_;
  };

  inline void assert_valid(Entity::Id id) const {
    assert(id.index() < entity_component_mask_.size() && "Entity::Id ID outside entity vector range");
    assert(entity_version_[id.index()] == id.version() && "Attempt to access Entity via a stale Entity::Id");
  }

  template <typename C>
  C *get_component_ptr(Entity::Id id) {
    assert(valid(id));
    BasePool *pool = component_pools_[C::family()];
    assert(pool);
    return static_cast<C*>(pool->get(id.index()));
  }

  template <typename C>
  const C *get_component_ptr(Entity::Id id) const {
    assert_valid(id);
    BasePool *pool = component_pools_[C::family()];
    assert(pool);
    return static_cast<const C*>(pool->get(id.index()));
  }

  ComponentMask component_mask(Entity::Id id) {
    assert_valid(id);
    return entity_component_mask_.at(id.index());
  }

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
  ComponentMask component_mask(const C *c) {
    return component_mask<C>();
  }

  template <typename C1, typename C2, typename ... Components>
  ComponentMask component_mask(const C1 *c1, const C2 *c2, Components * ... args) {
    return component_mask<C1>(c1) | component_mask<C2, Components ...>(c2, args...);
  }

  inline void accomodate_entity(uint32_t index) {
    if (entity_component_mask_.size() <= index) {
      entity_component_mask_.resize(index + 1);
      entity_version_.resize(index + 1);
      for (BasePool *pool : component_pools_)
        if (pool) pool->expand(index + 1);
    }
  }

  template <typename T>
  Pool<T> *accomodate_component() {
    BaseComponent::Family family = T::family();
    if (component_pools_.size() <= family) {
      component_pools_.resize(family + 1, nullptr);
    }
    if (!component_pools_[family]) {
      Pool<T> *pool = new Pool<T>();
      pool->expand(index_counter_);
      component_pools_[family] = pool;
    }
    return static_cast<Pool<T>*>(component_pools_[family]);
  }


  uint32_t index_counter_ = 0;

  EventManager &event_manager_;
  // Each element in component_pools_ corresponds to a Component::family().
  std::vector<BasePool*> component_pools_;
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


template <typename C, typename ... Args>
C *Entity::assign(Args && ... args) {
  assert(valid());
  return manager_->assign<C>(id_, std::forward<Args>(args) ...);
}

template <typename C>
void Entity::remove() {
  assert(valid() && component<C>());
  manager_->remove<C>(id_);
}

template <typename C>
C *Entity::component() {
  assert(valid());
  return manager_->component<C>(id_);
}

template <typename A, typename ... Args>
void Entity::unpack(A *&a, Args *& ... args) {
  assert(valid());
  manager_->unpack(id_, a, args ...);
}

inline bool Entity::valid() const {
  return manager_ && manager_->valid(id_);
}


}  // namespace entityx
