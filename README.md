# EntityX - A fast, type-safe C++ Entity-Component system

Entity-Component (EC) systems are a form of decomposition that completely decouples entity logic and data from the entity "objects" themselves. The [Evolve your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/) article provides a solid overview of EC systems and why you should use them.

EntityX is an EC system that uses C++11 features to provide type-safe component management, event delivery, etc. It was built during the creation of a 2D space shooter.

## Overview

In EntityX data associated with an entity is called a `Component`. `Systems` encapsulate logic and can use as many component types as necessary. An `EventManager` allows systems to interact without being tightly coupled. Finally, a `Manager` object ties all of the systems together for convenience.

As an example, a physics system might need *position* and *mass* data, while a collision system might only need *position* - the data would be logically separated into two components, but usable by any system. The physics system might emit *collision* events whenever two entities collide.

## Tutorial

Following is some skeleton code that implements `Position` and `Direction` components, a `MovementSystem` using these data components, and a `CollisionSystem` that emits `Collision` events when two entities collide.

### Entities

Entities are simply 64-bit numeric identifiers with which components are associated. Entity IDs are allocated by the `EntityManager`. Components are then associated with the entity, and can be queried or retrieved directly.

Creating an entity is as simple as:

```c++
EntityManager entities;

Entity entity = entities.create();
```

And destroying an entity is done with:

```c++
entity.destroy();
```

#### Implementation details

- Each Entity is a convenience class wrapping an Entity::Id.
- An Entity handle can be invalidated with `invalidate()`. This does not affect the underlying entity.
- When an entity is destroyed the manager adds its ID to a free list and invalidates the Entity handle.
- When an entity is created IDs are recycled from the free list before allocating new ones.

### Components (entity data)

The idea with ECS is to not have any functionality in the component. All logic should be contained in Systems.

To that end Components are typically [POD types](http://en.wikipedia.org/wiki/Plain_Old_Data_Structures) containing self-contained sets of related data. Implementations are [curiously recurring template pattern](http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) (CRTP) subclasses of `Component<T>`.

#### Creating components

As an example, position and direction information might be represented as:

```c++
struct Position : Component<Position> {
  Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  float x, y;
};

struct Direction : Component<Direction> {
  Direction(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  float x, y;
};
```

#### Assigning components to entities

To associate a component with a previously created entity call ``Entity::assign<C>()`` with the component type, and any component constructor arguments:

```c++
// Assign a Position with x=1.0f and y=2.0f to "entity"
entity.assign<Position>(1.0f, 2.0f);
```

You can also assign existing instances of components:

```c++
boost::shared_ptr<Position> position = boost::make_shared<Position>(1.0f, 2.0f);
entity.assign(position);
```

#### Querying entities and their components

To query all entities with a set of components assigned, use ``EntityManager::entities_with_components()``. This method will return only those entities that have *all* of the specified components associated with them, assigning each component pointer to the corresponding component instance:

```c++
for (auto entity : entities.entities_with_components<Position, Direction>()) {
  boost::shared_ptr<Position> position = entity.component<Position>();
  boost::shared_ptr<Direction> direction = entity.component<Direction>();

  // Do things with entity, position and direction.
}
```

To retrieve a component associated with an entity use ``Entity::component<C>()``:

```c++
boost::shared_ptr<Position> position = entity.component<Position>();
if (position) {
  // Do stuff with position
}
```

#### Implementation notes

- Components must provide a no-argument constructor.
- The current implementation can handle up to 64 components in total. This can be extended with little effort.

### Systems (implementing behavior)

Systems implement behavior using one or more components. Implementations are subclasses of `System<T>` and *must* implement the `update()` method, as shown below.

A basic movement system might be implemented with something like the following:

```c++
struct MovementSystem : public System<MovementSystem> {
  void update(EntityManager &es, EventManager &events, double dt) override {
    for (auto entity : es.entities_with_components<Position, Direction>()) {
      boost::shared_ptr<Position> position = entity.component<Position>();
      boost::shared_ptr<Direction> direction = entity.component<Direction>();

      position->x += direction->x * dt;
      position->y += direction->y * dt;
    }
  }
};
```

### Events (communicating between systems)

Events are objects emitted by systems, typically when some condition is met. Listeners subscribe to an event type and will receive a callback for each event object emitted. An ``EventManager`` coordinates subscription and delivery of events between subscribers and emitters. Typically subscribers will be other systems, but need not be.
Events are not part of the original ECS pattern, but they are an efficient alternative to component flags for sending infrequent data.

As an example, we might want to implement a very basic collision system using our ``Position`` data from above.

#### Creating event types

First, we define the event type, which for our example is simply the two entities that collided:

```c++
struct Collision : public Event<Collision> {
  Collision(Entity left, Entity right) : left(left), right(right) {}

  Entity left, right;
};
```

#### Emitting events

Next we implement our collision system, which emits ``Collision`` objects via an ``EventManager`` instance whenever two entities collide.

```c++
class CollisionSystem : public System<CollisionSystem> {
 public:
  void update(EntityManager &es, EventManager &events, double dt) override {
    boost::shared_ptr<Position> left_position, right_position;
    for (auto left_entity : es.entities_with_components<Position>()) {
      for (auto right_entity : es.entities_with_components<Position>()) {
        if (collide(left_position, right_position)) {
          events.emit<Collision>(left_entity, right_entity);
        }
      }
    }
  }
};
```

#### Subscribing to events

Objects interested in receiving collision information can subscribe to ``Collision`` events by first subclassing the CRTP class ``Receiver<T>``:

```c++
struct DebugSystem : public System<DebugSystem>, Receiver<DebugSystem> {
  void configure(EventManager &event_manager) {
    event_manager.subscribe<Collision>(*this);
  }

  void receive(const Collision &collision) {
    LOG(DEBUG) << "entities collided: " << collision.left << " and " << collision.right << endl;
  }
};
```

#### Builtin events

Several events are emitted by EntityX itself:

- `EntityCreatedEvent` - emitted when a new Entity has been created.
  - `Entity entity` - Newly created Entity.
- `EntityDestroyedEvent` - emitted when an Entity is *about to be* destroyed.
  - `Entity entity` - Entity about to be destroyed.
- `ComponentAddedEvent<T>` - emitted when a new component is added to an entity.
  - `Entity entity` - Entity that component was added to.
  - `boost::shared_ptr<T> component` - The component added.

#### Implementation notes

- There can be more than one subscriber for an event; each one will be called.
- Event objects are destroyed after delivery, so references should not be retained.
- A single class can receive any number of types of events by implementing a ``receive(const EventType &)`` method for each event type.
- Any class implementing `Receiver` can receive events, but typical usage is to make `System`s also be `Receiver`s.

### Manager (tying it all together)

Managing systems, components and entities can be streamlined by subclassing `Manager`. It is not necessary, but it provides callbacks for configuring systems, initializing entities, and so on.

To use it, subclass `Manager` and implement `configure()`, `initialize()` and `update()`:

```c++
class GameManager : public Manager {
 protected:
  void configure() {
    system_manager.add<DebugSystem>();
    system_manager.add<MovementSystem>();
    system_manager.add<CollisionSystem>();
  }

  void initialize() {
    // Create some entities in random locations heading in random directions
    for (int i = 0; i < 100; ++i) {
      Entity entity = entity_manager.create();
      entity.assign<Position>(rand() % 100, rand() % 100);
      entity.assign<Direction>((rand() % 10) - 5, (rand() % 10) - 5);
    }
  }

  void update(double dt) {
    system_manager.update<MovementSystem>(dt);
    system_manager.update<CollisionSystem>(dt);
  }
};
```

## Installation

EntityX has the following build and runtime requirements:

- A C++ compiler that supports a basic set of C++11 features (ie. recent clang, recent gcc, but **NOT** Visual C++).
- [CMake](http://cmake.org/)
- [Boost](http://boost.org) `1.48.0` or higher (links against `boost::signals`).
- [Glog](http://code.google.com/p/google-glog/) (tested with `0.3.2`).
- [GTest](http://code.google.com/p/googletest/) (needed for testing only)

**Note:** GTest is no longer installable directly through Homebrew. You can use [this formula](https://raw.github.com/mxcl/homebrew/2bf506e3d90254f81a669a0216f33b2f09589028/Library/Formula/gtest.rb) to install it manually.
For Debian Linux, install libgtest-dev and then see /usr/share/doc/libgtest-dev/README.Debian.

Once these dependencies are installed you should be able to build and install EntityX as follows. BUILD_TESTING is false by default.

```c++
mkdir build
cd build
cmake [-DBUILD_TESTING=true] ..
make
make install
```

EntityX has currently only been tested on Mac OSX (Lion and Mountain Lion), and Linux Debian. Reports and patches for builds on other platforms are welcome.
