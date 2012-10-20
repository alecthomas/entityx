# EntityX - A fast, type-safe C++ Entity-Component system

Entity-Component (EC) systems decouple entity behavior from the entity objects themselves. The [Evolve your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/) article provides a solid overview of EC systems.

EntityX is an EC system that uses C++11 features to provide type-safe component management, event delivery, etc.

## Overview

In EntityX, data is associated with entities through components. This data is then used by systems to implement behavior. Behavior systems can utilize as many types of data as necessary. As an example, a physics system might need *position* and *mass* data, while a collision system might only need *position* - the data would be logically separated, but usable by any system.

Finally, an event system ties behavior systems together, allowing them to interact without tight coupling.

### Entities

Entities are simply 64-bit numeric identifiers with which behaviors are associated. Entity IDs are allocated by the `EntityManager`, and all entities assigned particular types of data can be retrieved.

### Components (entity data)

Components are typically POD types containing self-contained sets of related data. Implementations are [CRTP](http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) subclasses of `Component<T>`.

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

### Systems (implementing behavior)

Systems implement behavior using one or more components. Implementations are [CRTP](http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) subclasses of `System<T>`. 

A basic movement system might be implemented with something like the following:

```
class MovementSystem : public System<MovementSystem> {
 public:
  MovementSystem() {}

  void update(EntityManager &es, EventManager &events, double) override {
    auto entities = es.entities_with_components<Position, Direction>();
    Position *position;
    Direction *direction;
    for (auto entity : entities) {
      es.unpack<Position, Direction>(entity, position, direction);
      position->x += direction->x;
      position->y += direction->y;
    }
  }
};
```

### Events (communicating between systems)

Events are objects emitted by systems, typically when some condition is met. Listeners subscribe to an event type and will receive a callback for each event object emitted. An ``EventManager`` coordinates subscription and delivery of events between subscribers and emitters. Typically subscribers will be other systems, but need not be.

As an example, we might want to implement a very basic collision system using our ``Position` data from above.

First, we define the event type, which for our example is simply the two entities that collided:

```
struct Collision : public Event<Collision> {
  Collision(Entity left, Entity right) : left(left), right(right) {}
  
  Entity left, right;
};
```

Next we implement our collision system, which emits ``Collision`` objects via an ``EventManager`` instance whenever two entities collide.

```
class CollisionSystem : public System<CollisionSystem> {
 public:
  CollisionSystem(EventManager &events) : events_(events) {}

  void update(EntityManager &es, EventManager &events, double dt) override {
    Position *left_position, *right_position;
    auto left_entities = es.entities_with_components<Position>(),
         right_entities = es.entities_with_components<Position>();

    for (auto left_entity : left_entities) {
      es.unpack<Position>(left_entity, left_position);
      for (auto right_entity : right_entities) {
        es.unpack<Position>(right_entity, right_position);
        if (collide(left_position, right_position)) {
          events_.emit<Collision>(left_entity, right_entity);
        }
      }
    }
  }
  
 private:
  EventManager &events_;
};
```

Objects interested in receiving collision can subscribe to ``Collision`` events by first subclassing the CRTP class ``Receiver<T>``:

```
struct DebugCollisions : public Receiver<DebugCollisions> {
  void receive(const Collision &collision) {
    LOG(DEBUG) << "entities collided: " << collision.left << " and " << collision.right << endl;
  }
};
```

Finally, we subscribe our receiver to collision events:

```
// Setup code (typically global)
EventManager events;
CollisionSystem collisions(events);
DebugCollisions debug_collisions;

// Subscribe to collisions
events.subscribe<Collision>(debug_collisions);
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