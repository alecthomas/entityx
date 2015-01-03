#pragma once
#include "entityx.hh"
#include "system.hh"

namespace entityx {

template <typename ... Cs>
class EntitySystemManager {
public:
protected:
  typedef entityx::Components<Cs...> Components;
  typedef entityx::EntityX<Components, entityx::ColumnStorage<Components>> EntityManager;
  template <typename T> using Component = typename EntityManager::template Component<T>;
  using Entity = typename EntityManager::Entity;
  
  EntityManager entities_;
  SystemManager<EntityManager> systems_ = SystemManager<EntityManager>(entities_);
};
} // namespace entityx
