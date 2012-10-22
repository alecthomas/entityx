# EntityX - A fast, type-safe C++ Entity-Component system

Entity-Component (EC) systems are a form of decomposition that completely decouple entity logic and data from the entity "objects" themselves. The [Evolve your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/) article provides a solid overview of EC systems.

EntityX is an EC system that uses C++11 features to provide type-safe component management, event delivery, etc.

## Overview

In EntityX data associated with an entity is called a `Component`. `Systems` use components to implement behavior and can utilize as many components as necessary. An `EventManager` allows systems to interact without being tightly coupled. Finally, a `World` object ties all of the systems together for convenience.

As an example, a physics system might need *position* and *mass* data, while a collision system might only need *position* - the data would be logically separated into two components, but usable by any system. The physics system might emit *collision* events whenever two entities collide.

## Tutorial



### Entities

Entities are simply 64-bit numeric identifiers with which components are associated. Entity IDs are allocated by the `EntityManager`. Components are then associated with the entity, and can be queried or retrieved directly.

Creating an entity is as simple as:

```
EntityManager entities;

Entity entity = entities.create();
```

### Components (entity data)

Components are typically POD types containing self-contained sets of related data. Implementations are [curiously recurring template pattern](http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) (CRTP) subclasses of `Component<T>`.

#### Creating components

As an example, position and direction information might be represented as:

```
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

To associate a component with a previously created entity call ``EntityManager::assign<C>()`` with the component type, the entity, and any component constructor arguments:

```
// Assign a Position with x=1.0f and y=2.0f to "entity"
entities.assign<Position>(entity, 1.0f, 2.0f);
```

You can also assign existing instances of components:

```
boost::shared_ptr<Position> position = boost::make_shared<Position>(1.0f, 2.0f);
entities.assign(entity, position);
```

#### Querying entities and their components

To retrieve a component associated with an entity use ``EntityManager::component()``:

```
boost::shared_ptr<Position> position = entities.component<Position>();
if (position) {
  // Do stuff with position
}
```

To query all components with a set of components assigned use ``EntityManager::entities_with_components()``. This method will return only those entities that have *all* of the specified components associated with them, assigning each component pointer to the corresponding component instance:

```
boost::shared_ptr<Position> position;
boost::shared_ptr<Direction> direction;
for (auto entity : entities.entities_with_components(position, direction)) {
  // Do things with entity ID, position and direction.
}
```

### Systems (implementing behavior)

Systems implement behavior using one or more components. Implementations are subclasses of `System<T>` and *must* implement the `update()` method, as shown below.

A basic movement system might be implemented with something like the following:

```
struct MovementSystem : public System<MovementSystem> {
  void update(EntityManager &es, EventManager &events, double dt) override {
    Position *position;
    Direction *direction;
    for (auto entity : es.entities_with_components(position, direction)) {
      position->x += direction->x;
      position->y += direction->y;
    }
  }
};
```

### Events (communicating between systems)

Events are objects emitted by systems, typically when some condition is met. Listeners subscribe to an event type and will receive a callback for each event object emitted. An ``EventManager`` coordinates subscription and delivery of events between subscribers and emitters. Typically subscribers will be other systems, but need not be.

As an example, we might want to implement a very basic collision system using our ``Position` data from above.

#### Creating event types

First, we define the event type, which for our example is simply the two entities that collided:

```
struct Collision : public Event<Collision> {
  Collision(Entity left, Entity right) : left(left), right(right) {}
  
  Entity left, right;
};
```

#### Emitting events

Next we implement our collision system, which emits ``Collision`` objects via an ``EventManager`` instance whenever two entities collide.

```
class CollisionSystem : public System<CollisionSystem> {
 public:
  void update(EntityManager &es, EventManager &events, double dt) override {
    boost::shared_ptr<Position> left_position, right_position;
    for (auto left_entity : es.entities_with_components(left_position)) {
      for (auto right_entity : es.entities_with_components(right_position)) {
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

```
struct DebugCollisions : public Receiver<DebugCollisions> {
  void receive(const Collision &collision) {
    LOG(DEBUG) << "entities collided: " << collision.left << " and " << collision.right << endl;
  }
};
```

Note that a single class can receive any number of types of events by implementing a ``receive(const EventType &)`` method for each event type.

Finally, we subscribe our receiver to collision events:

```
// Setup code (typically global)
EventManager events;
CollisionSystem collisions(events);
DebugCollisions debug_collisions;

// Subscribe to collisions
events.subscribe<Collision>(debug_collisions);
```

### World (tying it all together)

Managing systems, components and entities can be streamlined by subclassing `World`. It is not necessary, but it provides callbacks for configuring systems, initializing entities and the world, and so on.

To use it, subclass `World` and implement `configure()`, `initialize()` and `update()`:

```
class GameWorld : public World {
 protected:
  void configure() {
    system_manager.add<MovementSystem>();
    system_manager.add<CollisionSystem>();
  }
  
  void initialize() {
    // Create some entities in random locations heading in random directions
    for (int i = 0; i < 100; ++i) {
      Entity entity = entity_manager.create();
      entity_manager.assign<Position>(entity, rand() % 100, rand() % 100);
      entity_manager.assign<Direction>(entity, (rand() % 10) - 5, (rand() % 10) - 5);
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

- A C++ compiler that supports a basic set of C++11 features (eg. recent clang, recent gcc).
- [CMake](http://cmake.org/)
- [Boost](http://boost.org) `1.48.0` or higher (links against `boost::signals`).
- [Glog](http://code.google.com/p/google-glog/) (tested with `0.3.2`).
- [GTest](http://code.google.com/p/googletest/)

Once these dependencies are installed you should be able to build and install EntityX with:

```
mkdir build && cd build && cmake .. && make && make test && make install
```

EntityX has currently only been tested on Mac OSX (Lion and Mountain Lion). Reports and patches for builds on other platforms are welcome.
