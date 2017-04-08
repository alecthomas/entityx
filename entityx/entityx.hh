#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>
#include <bitset>
#include <type_traits>
#include <cassert>
#include <tuple>
#include <cstdlib>
#include <cstdint>
#include <queue>
#include <functional>


namespace entityx {

namespace details {

template <typename T, typename... Ts>
struct get_component_index;

template <typename T, typename... Ts>
struct get_component_index<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};

template <typename T, typename Tail, typename... Ts>
struct get_component_index<T, Tail, Ts...> :
    std::integral_constant<std::size_t, 1 + get_component_index<T, Ts...>::value> {};

template <typename T, typename... Ts>
struct get_component_size;

template <typename T, typename... Ts>
struct get_component_size<T, T, Ts...> : std::integral_constant<std::size_t, sizeof(T)> {};

template <typename T, typename Tail, typename... Ts>
struct get_component_size<T, Tail, Ts...> :
    std::integral_constant<std::size_t, get_component_size<T, Ts...>::value> {};

template <typename T, typename... Ts>
struct is_valid_component;

template <typename T, typename... Ts>
struct is_valid_component<T, T, Ts...> : std::true_type {};

template <typename T, typename Tail, typename... Ts>
struct is_valid_component<T, Tail, Ts...> : is_valid_component<T, Ts...>::type {};

template <typename T>
struct is_valid_component<T> {
    // condition is always false, but should be dependant of T
    static_assert(sizeof(T) == 0, "type is not a component");
};

template <typename T, typename... Ts>
struct get_component_offset;

template <typename T, typename... Ts>
struct get_component_offset<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};

template <typename T, typename Tail, typename... Ts>
struct get_component_offset<T, Tail, Ts...> :
    std::integral_constant<std::size_t, sizeof(Tail) + get_component_offset<T, Ts...>::value> {};


template <typename... Ts>
struct get_component_size_sum;

template <typename T>
struct get_component_size_sum<T> : std::integral_constant<std::size_t, sizeof(T)> {};

template <typename T, typename... Ts>
struct get_component_size_sum<T, Ts...> :
    std::integral_constant<std::size_t, sizeof(T) + get_component_size_sum<Ts...>::value> {};


}  // namespace details


/**
 * Storage interface.
 */
struct Storage {
  // A hint that the backend should be capable of storing this number of entities.
  void resize(std::size_t entities);

  // Retrieve component C from entity or return nullptr.
  template <typename C> C *get(std::uint32_t entity);

  // Create component C on entity with the given constructor arguments.
  template <typename C, typename ... Args> C *create(std::uint32_t entity, Args && ... args);

  // Call the destructor for the component C of entity.
  template <typename C> void destroy(std::uint32_t entity);

  // Free all underlying storage.
  void reset();
};



/**
 * ContiguousStorage is a storage engine for EntityX that uses semi-contiguous
 * blocks of memory for holding all components for all entities.
 */
template <class Components, int CHUNK_SIZE = 8192, int INITIAL_CHUNKS = 8>
class ContiguousStorage {
public:
  ContiguousStorage() {
    resize(CHUNK_SIZE * INITIAL_CHUNKS);
  }
  ContiguousStorage(const ContiguousStorage &) = delete;

  void resize(std::size_t n) {
    while (n > blocks_.size() * CHUNK_SIZE) {
      char *block = new char[CHUNK_SIZE * Components::component_size_sum];
      blocks_.push_back(block);
    }
  }

  template <typename C>
  C *get(std::uint32_t entity) {
    assert(entity < blocks_.size() * CHUNK_SIZE);
    return static_cast<C*>(static_cast<void*>(
      blocks_[entity / CHUNK_SIZE]
      + (entity % CHUNK_SIZE) * Components::component_size_sum
      + Components::template component_offset<C>::value
    ));
  }

  template <typename C, typename ... Args>
  C *create(std::uint32_t entity, Args && ... args) {
    assert(entity < blocks_.size() * CHUNK_SIZE);
    return new(get<C>(entity)) C(std::forward<Args>(args) ...);
  }

  template <typename C>
  void destroy(std::uint32_t entity) {
    assert(entity < blocks_.size() * CHUNK_SIZE);
    get<C>(entity)->~C();
  }

  void reset() {
    for (char *block : blocks_) delete [] block;
    blocks_.resize(0);
  }

private:
  std::vector<char*> blocks_;
};





/**
 * Interaction with a static set of component types.
 */
template <typename ... Cs>
struct Components {
  template <typename T>
  struct is_component : details::is_valid_component<T, Cs...> {};

  template <typename C>
  struct component_index : details::get_component_index<C, Cs...> {};

  template <typename C>
  struct component_size : details::get_component_size<C, Cs...> {};

  template <typename C>
  struct component_offset : details::get_component_offset<C, Cs...> {};

  // Number of components in the set.
  static const std::size_t component_count = sizeof...(Cs);
  // sizeof() sum of all components.
  static const std::size_t component_size_sum = details::get_component_size_sum<Cs...>::value;

  static std::vector<std::size_t> component_sizes() {
    std::vector<std::size_t> sizes;
    Components::init_sizes<Cs...>(sizes, 0);
    return sizes;
  }

  template <class Storage>
  static void destroy(Storage &storage, const std::bitset<component_count> &mask, std::uint32_t index) {
    Components::destroy<Storage, Cs...>(storage, mask, index);
  }

  template <class Storage, class C>
  static void destroy(Storage &storage, const std::bitset<component_count> &mask, std::uint32_t index) {
    if (mask.test(component_index<C>::value)) {
      storage.template destroy<C>(index);
    }
  }

  template <class Storage, template <typename> class Component, typename ... Components>
  static std::tuple<Component<Cs> & ...> unpack(Storage &storage, std::uint32_t index, Component<Components> & ... components) {
    return std::forward_as_tuple(storage.template get<Components>(index)...);
  }

  template <typename Storage, typename Fn>
  static void apply(Storage &storage, const std::bitset<component_count> &mask, std::uint32_t index, Fn fn) {
    apply_inner<Storage, Fn, Cs...>(storage, mask, index, fn);
  }

private:
  template <typename Storage, typename Fn>
  static void apply_inner(Storage &storage, const std::bitset<component_count> &mask, std::uint32_t index, Fn fn) {
  }

  template <typename Storage, typename Fn, typename C0, typename ... Cn>
  static void apply_inner(Storage &storage, const std::bitset<component_count> &mask, std::uint32_t index, Fn fn) {
    if (mask.test(component_index<C0>::value)) {
      fn(*storage.template get<C0>(index));
    }
    apply_inner<Storage, Fn, Cn...>(storage, mask, index, fn);
  }


  template <class C>
  static void init_sizes(std::vector<std::size_t> &sizes, std::uint32_t index = 0) {
    (void)index;
    sizes.push_back(sizeof(C));
  }

  template <class C0, class C1, class ... Cn>
  static void init_sizes(std::vector<std::size_t> &sizes, std::uint32_t index = 0) {
    sizes.push_back(sizeof(C0));
    Components::init_sizes<C1, Cn...>(sizes, index + 1);
  }

  template <class Storage, class C, class C1, class ... Cn>
  static void destroy(Storage &storage, const std::bitset<component_count> &mask, std::uint32_t index) {
    Components::destroy<Storage, C>(storage, mask, index);
    Components::destroy<Storage, C1, Cn...>(storage, mask, index);
  }
};





/**
 * A bitmask of features togglable at compile time.
 */
enum FeatureFlags {
  /**
   * If provided, various observer methods will be available:
   * on_component_added/removed, on_entity_created/destroyed.
   */
  OBSERVABLE = 1
};





struct Id {
  Id() : id_(0) {}
  explicit Id(std::uint64_t id) : id_(id) {}
  Id(std::uint32_t index, std::uint32_t version) : id_(std::uint64_t(index) | std::uint64_t(version) << 32UL) {}

  std::uint64_t id() const { return id_; }

  bool operator == (const Id &other) const { return id_ == other.id_; }
  bool operator != (const Id &other) const { return id_ != other.id_; }
  bool operator < (const Id &other) const { return id_ < other.id_; }

  std::uint32_t index() const { return id_ & 0xffffffffUL; }
  std::uint32_t version() const { return id_ >> 32; }

private:
  std::uint64_t id_;
};





/**
 * The main entity management class.
 *
 * @tparam Components The Components template parameter must be a realised
 *                    type instance of Components<C...>.
 * @tparam Storage A type that implements the storage interface.
 * @tparam Features A bitmask of features to enable. See FeatureFlags.
 */
template <class Components, class Storage = ContiguousStorage<Components>, std::size_t Features = 0>
class EntityX {
private:
  template <typename T>
  struct is_component : Components::template is_component<T> {};

  template <typename C>
  struct component_index : Components::template component_index<C> {};

  template <typename C>
  struct component_size : Components::template component_size<C> {};

  static const std::size_t component_count = Components::component_count;

  template <typename C, typename R = void>
  using enable_if_component = typename std::enable_if<is_component<C>::value, R>::type;

public:
  typedef std::bitset<Components::component_count> ComponentMask;

  class Entity;
  /**
   * A Component<C> is a wrapper around an instance of a component.
   *
   * It provides safe access to components. The handle will be invalidated under
   * the following conditions:
   *
   * - If a component is removed from its host entity.
   * - If its host entity is destroyed.
   */
  template <typename C>
  class Component {
  public:
    typedef C ComponentType;

    Component() : manager_(nullptr) {}

    bool valid() const {return manager_ && manager_->template has_component<C>(id_); }
    operator bool () const { return valid(); }

    C *operator -> () { return get(); }
    const C *operator -> () const { return get(); }

    C *get() {
      assert(manager_ && "manager is null");
      return manager_->template component_ptr_<C>(id_);
    }
    const C *get() const {
      assert(manager_ && "manager is null");
      return manager_->template component_ptr_<C>(id_);
    }

    C &operator * () { return *get(); }
    const C &operator * () const { return *get(); }

    /** Remove the component from its entity and destroy it. */
    void remove() {
      assert(manager_ && "manager is null");
      manager_->template remove<C>(id_);
    }

    bool operator == (const Component<C> &other) const { return manager_ == other.manager_ && id_ == other.id_; }
    bool operator != (const Component<C> &other) const { return !(*this == other); }
    bool operator < (const Component<C> &other) const { return (id_ < other.id_); }

    Entity entity() {
      return {manager_, id_};
    }

  private:
    friend class EntityX;

    Component(EntityX *manager, Id id) :
        manager_(manager), id_(id) {}
    Component(const EntityX *manager, Id id) :
        manager_(const_cast<EntityX*>(manager)), id_(id) {}

    EntityX *manager_;
    Id id_;
  };


  /**
   * A convenience handle around an Id.
   *
   * If an entity is destroyed, any copies will be invalidated. Use valid() to
   * check for validity before using.
   *
   * Create entities with `EntityX`:
   *
   *     Entity entity = entity_manager->create();
   */
  class Entity {
  public:
    /**
     * Id of an invalid Entity.
     */
    static const Id INVALID;

    Entity() {}
    Entity(EntityX *manager, Id id) : manager_(manager), id_(id) {}
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

    bool operator < (const Entity &other) const {
      return other.id_ < id_;
    }

    /**
     * Is this Entity handle valid?
     */
    bool valid() const {
      return manager_ && manager_->valid(id_);
    }

    /**
     * Invalidate Entity handle, disassociating it from an EntityX and invalidating its ID.
     *
     * Note that this does *not* affect the underlying entity and its
     * components. Use destroy() to destroy the associated Entity and components.
     */
    void invalidate() {
      id_ = INVALID;
      manager_ = nullptr;
    }

    Id id() const { return id_; }

    template <typename C>
    enable_if_component<C, Component<C>> component() {
      assert(valid());
      return manager_->component<C>(id_);
    }

    template <typename C>
    enable_if_component<C, const Component<const C>> component() const {
      return manager_->component<C>(id_);
    }

    template <typename ... Cn>
    std::tuple<Component<Cn>...> components() {
      return manager_->components<Cn...>(id_);
    }

    template <typename ... Cn>
    std::tuple<Component<const Cn>...> components() const {
      return manager_->components<Cn...>();
    }

    template <typename C, typename ... Args>
    enable_if_component<C, Component<C>> assign(Args && ... args) {
      assert(valid());
      return manager_->assign<C>(id_, std::forward<Args>(args)...);
    }

    template <typename C>
    enable_if_component<C, Component<C>> assign_from_copy(const C &component) {
      assert(valid());
      return manager_->assign<C>(id_, std::forward<const C &>(component));
    }

    template <typename C>
    enable_if_component<C> remove() {
      assert(valid());
      manager_->remove<C>(id_);
    }

    /**
     * Destroy and invalidate this Entity.
     */
    void destroy() {
      manager_->destroy(id_);
      invalidate();
    }

    ComponentMask component_mask() const {
      return manager_->component_mask(id_);
    }

   private:
    EntityX *manager_ = nullptr;
    Id id_ = INVALID;
  };

  /// An iterator over a view of the entities in an EntityX.
  /// If All is true it will iterate over all valid entities and will ignore the entity mask.
  template <class Delegate, bool All = false>
  class ViewIterator : public std::iterator<std::input_iterator_tag, Id> {
   public:
    Delegate &operator ++() {
      ++i_;
      next();
      return *reinterpret_cast<Delegate*>(this);
    }
    bool operator == (const Delegate& rhs) const { return i_ == rhs.i_; }
    bool operator != (const Delegate& rhs) const { return i_ != rhs.i_; }
    Entity operator * () { return Entity(manager_, manager_->create_id(i_)); }
    const Entity operator * () const { return Entity(manager_, manager_->create_id(i_)); }

   protected:
    ViewIterator(EntityX *manager, uint32_t index)
        : manager_(manager), i_(index), capacity_(manager_->capacity()), free_cursor_(~0UL) {
      if (All) {
        std::sort(manager_->free_list_.begin(), manager_->free_list_.end());
        free_cursor_ = 0;
      }
    }
    ViewIterator(EntityX *manager, const ComponentMask &mask, uint32_t index)
        : manager_(manager), mask_(mask), i_(index), capacity_(manager_->capacity()), free_cursor_(~0UL) {
      if (All) {
        std::sort(manager_->free_list_.begin(), manager_->free_list_.end());
        free_cursor_ = 0;
      }
    }

    void next() {
      while (i_ < capacity_ && !predicate()) {
        ++i_;
      }

      if (i_ < capacity_) {
        Entity entity = manager_->get(manager_->create_id(i_));
        reinterpret_cast<Delegate*>(this)->next_entity(entity);
      }
    }

    inline bool predicate() {
      return (All && valid_entity()) || (manager_->entity_component_mask_[i_] & mask_) == mask_;
    }

    inline bool valid_entity() {
      const std::vector<uint32_t> &free_list = manager_->free_list_;
      if (free_cursor_ < free_list.size() && free_list[free_cursor_] == i_) {
        ++free_cursor_;
        return false;
      }
      return true;
    }

    EntityX *manager_;
    ComponentMask mask_;
    uint32_t i_;
    size_t capacity_;
    size_t free_cursor_;
  };

  template <bool All>
  class BaseView {
  public:
    class Iterator : public ViewIterator<Iterator, All> {
    public:
      Iterator(EntityX *manager,
        const ComponentMask &mask,
        uint32_t index) : ViewIterator<Iterator, All>(manager, mask, index) {
        ViewIterator<Iterator, All>::next();
      }

      void next_entity(Entity &entity) {
        (void)entity;
      }
    };


    Iterator begin() { return Iterator(manager_, mask_, 0); }
    Iterator end() { return Iterator(manager_, mask_, uint32_t(manager_->capacity())); }
    const Iterator begin() const { return Iterator(manager_, mask_, 0); }
    const Iterator end() const { return Iterator(manager_, mask_, manager_->capacity()); }

  private:
    friend class EntityX;

    explicit BaseView(EntityX *manager) : manager_(manager) { mask_.set(); }
    BaseView(EntityX *manager, ComponentMask mask) :
        manager_(manager), mask_(mask) {}

    EntityX *manager_;
    ComponentMask mask_;
  };

  typedef BaseView<false> View;
  typedef BaseView<true> AllView;

  template <typename ... ComponentsToUnpack>
  class UnpackingView {
   public:
    struct Unpacker {
      explicit Unpacker(Component<ComponentsToUnpack> & ... handles) :
          handles(std::tuple<Component<ComponentsToUnpack> & ...>(handles...)) {}

      void unpack(Entity &entity) const {
        unpack_<0, ComponentsToUnpack...>(entity);
      }

    private:
      template <int N, typename C>
      void unpack_(Entity &entity) const {
        std::get<N>(handles) = entity.template component<C>();
      }

      template <int N, typename C0, typename C1, typename ... Cn>
      void unpack_(Entity &entity) const {
        std::get<N>(handles) = entity.template component<C0>();
        unpack_<N + 1, C1, Cn...>(entity);
      }

      std::tuple<Component<ComponentsToUnpack> & ...> handles;
    };


    class Iterator : public ViewIterator<Iterator> {
    public:
      Iterator(EntityX *manager,
        const ComponentMask &mask,
        uint32_t index,
        const Unpacker &unpacker) : ViewIterator<Iterator>(manager, mask, index), unpacker_(unpacker) {
        ViewIterator<Iterator>::next();
      }

      void next_entity(Entity &entity) {
        unpacker_.unpack(entity);
      }

    private:
      const Unpacker &unpacker_;
    };


    Iterator begin() { return Iterator(manager_, mask_, 0, unpacker_); }
    Iterator end() { return Iterator(manager_, mask_, manager_->capacity(), unpacker_); }
    const Iterator begin() const { return Iterator(manager_, mask_, 0, unpacker_); }
    const Iterator end() const { return Iterator(manager_, mask_, manager_->capacity(), unpacker_); }


   private:
    friend class EntityX;

    UnpackingView(EntityX *manager, ComponentMask mask, Component<ComponentsToUnpack> & ... handles) :
        manager_(manager), mask_(mask), unpacker_(handles...) {}

    EntityX *manager_;
    ComponentMask mask_;
    Unpacker unpacker_;
  };


  EntityX() : owned_storage_(new Storage()), storage_(*owned_storage_.get()) {}
  explicit EntityX(Storage &storage) : storage_(storage) {}
  ~EntityX() {
    for (Entity entity : all_entities()) entity.destroy();
  }

  EntityX(const EntityX &) = delete;
  EntityX(EntityX &&) = delete;
  EntityX &operator = (const EntityX &) const = delete;

  /**
   * Number of assigned entities.
   */
  std::size_t size() const { return entity_component_mask_.size() - free_list_.size(); }
  /**
   * Number of entities available without reallocation.
   */
  std::size_t capacity() const { return entity_component_mask_.size(); }

  /**
   * Create a new Entity.
   **/
  Entity create();

  /**
   * Create many Entity's at once.
   *
   * Should be more efficient than create() for large numbers of entities.
   */
  std::vector<Entity> create_many(std::size_t count);

  /**
   * Destroy an existing Id and its associated Components.
   *
   * Emits EntityDestroyedEvent.
   */
  void destroy(Id entity);

  /**
   * Return true if the given entity ID is still valid.
   */
  bool valid(Id id) const;


  /**
   * Assign a Component to an Id, passing through Component constructor arguments.
   *
   *     Position &position = em.assign<Position>(e, x, y);
   *
   * @returns Smart pointer to newly created component.
   */
  template <typename C, typename ... Args>
  enable_if_component<C, Component<C>> assign(Id id, Args && ... args) {
    assert_valid(id);
    assert(!entity_component_mask_[id.index()].test(component_index<C>::value));

    // Placement new into the component pool.
    storage_.template create<C, Args ...>(id.index(), std::forward<Args>(args) ...);

    // Set the bit for this component.
    entity_component_mask_[id.index()].set(component_index<C>::value);

    // Create and return handle.
    Component<C> component(this, id);
    component_added_<C>(Entity(this, id), component);
    return component;
  }


  template <typename ... Cn>
  std::tuple<Component<Cn>...> components(Id id) {
    return std::make_tuple(component<Cn>(id)...);
  }


  template <typename ... Cn>
  std::tuple<Component<const Cn>...> components(Id id) const {
    return std::make_tuple(component<const Cn>(id)...);
  }


  /**
   * Remove a Component from an Id
   *
   * No-op if component is not present.
   */
  template <typename C>
  enable_if_component<C> remove(Id id) {
    assert_valid(id);
    if (!entity_component_mask_[id.index()].test(component_index<C>::value))
      return;

    const std::uint32_t index = id.index();
    Component<C> component(this, id);

    component_removed_<C>(Entity(this, id), component);
    // Call destructor.
    Components::template destroy<Storage, C>(storage_, entity_component_mask_[index], index);
    // Remove component bit.
    entity_component_mask_[id.index()].reset(component_index<C>::value);
  }


  /**
   * Retrieve a Component assigned to an Id.
   *
   * @returns Pointer to an instance of C, or nullptr if the Id does not have that Component.
   */
  template <typename C>
  enable_if_component<C, Component<C>> component(Id id) {
    return has_component<C>(id) ? Component<C>(this, id) : Component<C>();
  }

  template <typename C>
  enable_if_component<C, bool> has_component(Id id) {
    return valid(id) && entity_component_mask_[id.index()].test(component_index<C>::value);
  }

  ComponentMask component_mask(Id id) {
    assert_valid(id);
    return entity_component_mask_.at(id.index());
  }

  // Called whenever an entity is created.
  template <typename = std::enable_if<Features & OBSERVABLE>>
  void on_entity_created(std::function<void(Entity)> callback) {
    on_entity_created_ = callback;
  }

  // Called whenever an entity is destroyed.
  template <typename = std::enable_if<Features & OBSERVABLE>>
  void on_entity_destroyed(std::function<void(Entity)> callback) {
    on_entity_destroyed_ = callback;
  }

  // Called whenever a component of type Component is added to an entity.
  template <typename C, typename = std::enable_if<Features & OBSERVABLE && is_component<C>::value>>
  void on_component_added(std::function<void(Entity, Component<C>)> callback) {
    on_component_added_[component_index<C>::value] = [callback](Entity entity, void *ptr) {
      callback(entity, *reinterpret_cast<Component<C>*>(ptr));
    };
  }

  // Called whenever a component of type Component is removed from an entity.
  template <typename C, typename = std::enable_if<Features & OBSERVABLE && is_component<C>::value>>
  void on_component_removed(std::function<void(Entity, Component<C>)> callback) {
    on_component_removed_[component_index<C>::value] = [callback](Entity entity, void *ptr) {
      callback(entity, *reinterpret_cast<Component<C>*>(ptr));
    };
  }

  /**
   * Find Entities that have all of the specified components.
   *
   * @code
   * for (Entity entity : entity_manager.entities_with_components<Position, Direction>()) {
   *   Component<Position> position = entity.component<Position>();
   *   Component<Direction> direction = entity.component<Direction>();
   *
   *   ...
   * }
   * @endcode
   */
  template <typename ... ComponentsToFilter>
  View entities_with_components() {
    ComponentMask mask = component_mask<ComponentsToFilter ...>();
    return View(this, mask);
  }

  /**
   * Find Entities that have all of the specified Components and assign them
   * to the given parameters.
   *
   * @code
   * Component<Position> position;
   * Component<Direction> direction;
   * for (Entity entity : entity_manager.entities_with_components(position, direction)) {
   *   // Use position and component here.
   * }
   * @endcode
   */
  template <typename ... ComponentsToFilter>
  UnpackingView<ComponentsToFilter...> entities_with_components(Component<ComponentsToFilter> & ... components) {
    ComponentMask mask = component_mask<ComponentsToFilter...>();
    return UnpackingView<ComponentsToFilter...>(this, mask, components...);
  }

  /**
   * Call function with (Entity, ComponentsToFilter&...).
   *
   * @code
   * entity_manager.for_each([&](Entity entity, Body &body, Position &position) {
   *    // ...
   * })
   * @endcode
   */
  template <typename ... ComponentsToFilter, typename F>
  void for_each(F f) {
    for (auto entity : entities_with_components<ComponentsToFilter...>())
      f(entity, *(entity.template component<ComponentsToFilter>().get())...);
  }



  /**
   * Iterate over all *valid* entities (ie. not in the free list). Not fast,
   * so should generally only be used for debugging.
   *
   * @code
   * for (Entity entity : entity_manager.all_entities()) {}
   *
   * @return An iterator view over all valid entities.
   */
  AllView all_entities() {
    return AllView(this);
  }

  template <typename C>
  void unpack(Id id, Component<C> &a) {
    assert_valid(id);
    a = component<C>(id);
  }

  /**
   * Unpack components directly into handles.
   *
   * Components missing from the entity will be set to nullptr.
   *
   * Useful for fast bulk iterations.
   *
   * Component<Position> p;
   * Component<Direction> d;
   * unpack<Position, Direction>(e, p, d);
   */
  template <typename A, typename ... Args>
  void unpack(Id id, Component<A> &a, Component<Args> & ... args) {
    assert_valid(id);
    a = component<A>(id);
    unpack<Args ...>(id, args ...);
  }

  Entity get(Id id) {
    assert_valid(id);
    return Entity(this, id);
  }

  // Reset all entity and component data.
  void reset();

private:
  struct on_component_removed_proxy {
    on_component_removed_proxy(EntityX *em, Entity entity) : em(em), entity(entity) {}

    template <typename C>
    void operator () (C &component) {
      em->component_removed_(entity, Component<C>(em, entity.id()));
    }

  private:
    EntityX *em;
    Entity entity;
  };

  // These proxy functions are nooped at compile time .

  template <bool FeatureMask = Features>
  typename std::enable_if<FeatureMask & OBSERVABLE>::type
  entity_created_(Entity entity) { if (on_entity_created_) on_entity_created_(entity); }
  template <bool FeatureMask = Features>
  typename std::enable_if<!(FeatureMask & OBSERVABLE)>::type
  entity_created_(Entity) {}

  template <bool FeatureMask = Features>
  typename std::enable_if<FeatureMask & OBSERVABLE>::type
  entity_destroyed_(Entity entity) { if (on_entity_destroyed_) on_entity_destroyed_(entity); }
  template <bool FeatureMask = Features>
  typename std::enable_if<!(FeatureMask & OBSERVABLE)>::type
  entity_destroyed_(Entity) {}

  template <class C, bool FeatureMask = Features>
  typename std::enable_if<FeatureMask & OBSERVABLE>::type
  component_added_(Entity entity, Component<C> component) {
    if (on_component_added_[component_index<C>::value])
      on_component_added_[component_index<C>::value](entity, reinterpret_cast<void*>(&component));
  }
  template <class C, bool FeatureMask = Features>
  typename std::enable_if<!(FeatureMask & OBSERVABLE)>::type
  component_added_(Entity, Component<C>) {}

  template <class C, bool FeatureMask = Features>
  typename std::enable_if<FeatureMask & OBSERVABLE>::type
  component_removed_(Entity entity, Component<C> component) {
    if (on_component_removed_[component_index<C>::value])
      on_component_removed_[component_index<C>::value](entity, reinterpret_cast<void*>(&component));
  }
  template <class C, bool FeatureMask = Features>
  typename std::enable_if<!(FeatureMask & OBSERVABLE)>::type
  component_removed_(Entity, Component<C>) {}

  template <typename C>
  ComponentMask component_mask() {
    ComponentMask mask;
    mask.set(component_index<C>::value);
    return mask;
  }

  template <typename C1, typename C2, typename ... Cn>
  ComponentMask component_mask() {
    return component_mask<C1>() | component_mask<C2, Cn ...>();
  }

  /**
   * Create an Entity::Id for a slot.
   *
   * NOTE: Does *not* check for validity, but the Entity::Id constructor will
   * fail if the ID is invalid.
   */
  Id create_id(uint32_t index) const {
    return Id(index, entity_version_[index]);
  }


  inline void assert_valid(Id id) const {
    assert(id.index() < entity_component_mask_.size() && "Id ID outside entity vector range");
    assert(entity_version_[id.index()] == id.version() && "Attempt to access Entity via a stale Id");
  }

  template <typename C>
  inline enable_if_component<C> assert_valid_component(Id id) {
    assert_valid(id);
    assert(entity_component_mask_[id.index()].test(component_index<C>::value) && "Component is not set on entity.");
  }

  void accomodate_entity_(std::uint32_t index);

  template <typename C>
  C *component_ptr_(Id id) {
    assert_valid_component<C>(id);
    return storage_.template get<C>(id.index());
  }

  template <typename C>
  const C *component_ptr_(Id id) const {
    assert_valid_component<C>(id);
    return static_cast<const C*>(storage_.template get<C>(id.index()));
  }


  std::unique_ptr<Storage> owned_storage_;
  Storage &storage_;
  std::uint32_t index_counter_ = 0;
  // Bitmask of components associated with each entity. Index into the vector is the Id.
  std::vector<ComponentMask> entity_component_mask_;
  // Vector of entity version numbers. Incremented each time an entity is destroyed
  std::vector<std::uint32_t> entity_version_;
  // List of available entity slots.
  std::vector<std::uint32_t> free_list_;

  std::function<void(Entity)> on_entity_created_, on_entity_destroyed_;
  std::function<void(Entity, void*)>
    on_component_added_[component_count],
    on_component_removed_[component_count];
};





template <class Components, class Storage, std::size_t Features>
inline void EntityX<Components, Storage, Features>::accomodate_entity_(std::uint32_t index) {
  if (entity_component_mask_.size() <= index) {
    entity_component_mask_.resize(index + 1);
    entity_version_.resize(index + 1);
    storage_.resize(index + 1);
  }
}




template <class Components, class Storage, std::size_t Features>
inline typename EntityX<Components, Storage, Features>::Entity
EntityX<Components, Storage, Features>::create() {
  std::uint32_t index, version;
  if (free_list_.empty()) {
    index = index_counter_++;
    accomodate_entity_(index);
    version = entity_version_[index] = 1;
  } else {
    index = free_list_.back();
    free_list_.pop_back();
    version = entity_version_[index];
  }
  Entity entity(this, Id(index, version));
  entity_created_(entity);
  return entity;
}





template <class Components, class Storage, std::size_t Features>
std::vector<typename EntityX<Components, Storage, Features>::Entity>
EntityX<Components, Storage, Features>::create_many(std::size_t count) {
  std::vector<Entity> entities;
  entities.reserve(count);
  std::uint32_t index, version;

  // Consume free list.
  while (count != 0 && !free_list_.empty()) {
    index = free_list_.back();
    free_list_.pop_back();
    version = entity_version_[index];
    Entity entity(this, Id(index, version));
    entity_created_(entity);
    entities.push_back(entity);
    --count;
  }

  // Allocate a bunch of capacity for entities all at once.
  accomodate_entity_(index_counter_ + count);
  while (count != 0) {
    index = index_counter_++;
    version = entity_version_[index] = 1;
    Entity entity(this, Id(index, version));
    entity_created_(entity);
    entities.push_back(entity);
    --count;
  }
  return entities;
}





template <class Components, class Storage, std::size_t Features>
inline void EntityX<Components, Storage, Features>::destroy(Id id) {
  assert_valid(id);
  Entity entity(this, id);
  const std::uint32_t index = id.index();
  ComponentMask mask = entity_component_mask_[index];
  // Notify on_component_removed() callback.
  Components::apply(storage_, mask, index, on_component_removed_proxy(this, entity));
  // Notify on_entity_destroyed() callback.
  entity_destroyed_(entity);
  Components::template destroy<Storage>(storage_, mask, index);
  entity_component_mask_[index].reset();
  entity_version_[index]++;
  free_list_.push_back(index);
}




template <class Components, class Storage, std::size_t Features>
void EntityX<Components, Storage, Features>::reset() {
  for (Entity entity : all_entities()) entity.destroy();
  storage_.reset();
  entity_component_mask_.clear();
  entity_version_.clear();
  free_list_.clear();
  index_counter_ = 0;
}




template <class Components, class Storage, std::size_t Features>
bool EntityX<Components, Storage, Features>::valid(Id id) const {
  return id.index() < entity_version_.size() && entity_version_[id.index()] == id.version();
}


template <class Components, class Storage, std::size_t Features>
const Id EntityX<Components, Storage, Features>::Entity::INVALID = Id();






}  // namespace entityx
