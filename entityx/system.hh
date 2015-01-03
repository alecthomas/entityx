#pragma once
#include <unordered_map>
#include <cassert>
#include <memory>
#include <bits/stl_bvector.h>
#include <typeindex>
#include <typeinfo>

namespace entityx {

template <class EntityManager>
class System {
public:
  virtual ~System() {}
  virtual void Update(EntityManager& entity_manager, double dt) = 0;
};

template <class EntityManager>
class SystemManager {
public:
  SystemManager(EntityManager& entity_manager) : entity_manager_(entity_manager) {}

  /**
  * Add a System to the SystemManager.
  *
  * Must be called before Systems can be used.
  *
  * eg.
  * std::shared_ptr<MovementSystem> movement = entityx::make_shared<MovementSystem>();
  * system.add(movement);
  */
  template <template <typename> class S>
  void Add(std::shared_ptr<S<EntityManager>> system) {
    //systems_.insert(std::make_pair(std::type_index(typeid(S<EntityManager>)), system));
    systems_[std::type_index(typeid(S<EntityManager>))] = system;
  }

  /**
  * Add a System to the SystemManager.
  *
  * Must be called before Systems can be used.
  *
  * eg.
  * auto movement = system.add<MovementSystem>();
  */
  template <template <typename> class S, typename ... Args>
  std::shared_ptr<S<EntityManager>> Add(Args && ... args) {
    std::shared_ptr<S<EntityManager>> s(new S<EntityManager>(std::forward<Args>(args) ...));
    Add(s);
    return s;
  }

  /**
  * Retrieve the registered System instance, if any.
  *
  *   std::shared_ptr<CollisionSystem> collisions = systems.system<CollisionSystem>();
  *
  * @return System instance or empty shared_std::shared_ptr<S>.
  */
  template <template <typename> class S>
  std::shared_ptr<S<EntityManager>> system() {
    //return systems_[std::type_index(typeid(S<EntityManager>))];
    auto it = systems_.find(std::type_index(typeid(S<EntityManager>)));
    assert(it != systems_.end());
    return it == systems_.end() ? std::shared_ptr<S<EntityManager>>()
				: std::shared_ptr<S<EntityManager>>(std::static_pointer_cast<S<EntityManager>>(it->second));
  }

  /**
  * Call the System::update() method for a registered system.
  */
  template <template <typename> class S>
  void Update(double dt) {
    //assert(initialized_ && "SystemManager::configure() not called");
    std::shared_ptr<S<EntityManager>> s = system<S<EntityManager>>();
    s->update(entity_manager_, dt);
  }

private:
  EntityManager& entity_manager_;
  std::unordered_map<std::type_index, std::shared_ptr<System<EntityManager>>> systems_;
};
} // namespace entityx
