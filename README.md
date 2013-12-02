# EntityX - A fast, type-safe C++ Entity Component System 

[![Build Status](https://travis-ci.org/alecthomas/entityx.png)](https://travis-ci.org/alecthomas/entityx)

Entity Component Systems (ECS) are a form of decomposition that completely decouples entity logic and data from the entity "objects" themselves. The [Evolve your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/) article provides a solid overview of EC systems and why you should use them.

EntityX is an EC system that uses C++11 features to provide type-safe component management, event delivery, etc. It was built during the creation of a 2D space shooter.

## Downloading

You can acquire stable releases [here](https://github.com/alecthomas/entityx/releases).

Alternatively, you can check out the current development version with:

```
git checkout https://github.com/alecthomas/entityx.git
```

See [below](#installation) for installation instructions.

## Contact

EntityX now has a mailing list! Send a mail to [entityx@librelist.com](mailto:entityx@librelist.com) to subscribe. Instructions will follow.

You can also contact me directly via [email](mailto:alec@swapoff.org) or [Twitter](https://twitter.com/alecthomas).


## Recent Notable Changes

- 2013-10-29 - Boost has been removed as a primary dependency for builds not using python.
- 2013-08-21 - Remove dependency on `boost::signal` and switch to embedded [Simple::Signal](http://timj.testbit.eu/2013/cpp11-signal-system-performance/).
- 2013-08-18 - Destroying an entity invalidates all other references
- 2013-08-17 - Python scripting, and a more robust build system

See the [ChangeLog](https://github.com/alecthomas/entityx/blob/master/CHANGES.md) for details.

## Overview

In EntityX data associated with an entity is called a `entityx::Component`. `Systems` encapsulate logic and can use as many component types as necessary. An `entityx::EventManager` allows systems to interact without being tightly coupled. Finally, a `Manager` object ties all of the systems together for convenience.

As an example, a physics system might need *position* and *mass* data, while a collision system might only need *position* - the data would be logically separated into two components, but usable by any system. The physics system might emit *collision* events whenever two entities collide.

## Tutorial

Following is some skeleton code that implements `Position` and `Direction` components, a `MovementSystem` using these data components, and a `CollisionSystem` that emits `Collision` events when two entities collide.

To start with, add the following line to your source file:

```c++
#include "entityx/entityx.h"
```

### Entities

An `entityx::Entity` is a convenience class wrapping an opaque `uint64_t` value allocated by the `entityx::EntityManager`. Each entity has a set of components associated with it that can be added, queried or retrieved directly.

Creating an entity is as simple as:

```c++
entityx::ptr<entityx::EventManager> events(new entityx::EventManager());
entityx::ptr<entityx::EntityManager> entities(new entityx::EntityManager(event));

entityx::Entity entity = entities->create();
```

And destroying an entity is done with:

```c++
entity.destroy();
```

#### Implementation details

- Each `entityx::Entity` is a convenience class wrapping an `entityx::Entity::Id`.
- An `entityx::Entity` handle can be invalidated with `invalidate()`. This does not affect the underlying entity.
- When an entity is destroyed the manager adds its ID to a free list and invalidates the `entityx::Entity` handle.
- When an entity is created IDs are recycled from the free list before allocating new ones.
- An `entityx::Entity` ID contains an index and a version. When an entity is destroyed, the version associated with the index is incremented, invalidating all previous entities referencing the previous ID.
- EntityX uses a reference counting smart pointer`entityx::ptr<T>` to manage object lifetimes. As a general rule, passing a pointer to any EntityX method will convert to a smart pointer and take ownership. To maintain your own reference to the pointer you will need to wrap it in `entityx::ptr<T>`.

### Components (entity data)

The general idea with the EntityX interpretation of ECS is to have as little functionality in components as possible. All logic should be contained in Systems.

To that end Components are typically [POD types](http://en.wikipedia.org/wiki/Plain_Old_Data_Structures) consisting of self-contained sets of related data. Implementations are [curiously recurring template pattern](http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) (CRTP) subclasses of `entityx::Component<T>`.

#### Creating components

As an example, position and direction information might be represented as:

```c++
struct Position : entityx::Component<Position> {
  Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  float x, y;
};

struct Direction : entityx::Component<Direction> {
  Direction(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  float x, y;
};
```

#### Assigning components to entities

To associate a component with a previously created entity call ``entityx::Entity::assign<C>()`` with the component type, and any component constructor arguments:

```c++
// Assign a Position with x=1.0f and y=2.0f to "entity"
entity.assign<Position>(1.0f, 2.0f);
```

You can also assign existing instances of components:

```c++
entityx::ptr<Position> position(new Position(1.0f, 2.0f));
entity.assign(position);
```

#### Querying entities and their components

To query all entities with a set of components assigned, use ``entityx::EntityManager::entities_with_components()``. This method will return only those entities that have *all* of the specified components associated with them, assigning each component pointer to the corresponding component instance:

```c++
for (auto entity : entities->entities_with_components<Position, Direction>()) {
  entityx::ptr<Position> position = entity.component<Position>();
  entityx::ptr<Direction> direction = entity.component<Direction>();

  // Do things with entity, position and direction.
}
```

To retrieve a component associated with an entity use ``entityx::Entity::component<C>()``:

```c++
entityx::ptr<Position> position = entity.component<Position>();
if (position) {
  // Do stuff with position
}
```

#### Component dependencies

In the case where a component has dependencies on other components, a helper class exists that will automatically create these dependencies.

eg. The following will also add `Position` and `Direction` components when a `Physics` component is added to an entity.

```c++
#include "entityx/deps/Dependencies.h"

system_manager->add<entityx::deps::Dependency<Physics, Position, Direction>>();
```

#### Implementation notes

- Components must provide a no-argument constructor.
- The default implementation can handle up to 64 components in total. This can be extended by changing the `entityx::EntityManager::MAX_COMPONENTS` constant.

### Systems (implementing behavior)

Systems implement behavior using one or more components. Implementations are subclasses of `System<T>` and *must* implement the `update()` method, as shown below.

A basic movement system might be implemented with something like the following:

```c++
struct MovementSystem : public System<MovementSystem> {
  void update(entityx::ptr<entityx::EntityManager> es, entityx::ptr<entityx::EventManager> events, double dt) override {
    for (auto entity : es->entities_with_components<Position, Direction>()) {
      entityx::ptr<Position> position = entity.component<Position>();
      entityx::ptr<Direction> direction = entity.component<Direction>();

      position->x += direction->x * dt;
      position->y += direction->y * dt;
    }
  };
};
```

### Events (communicating between systems)

Events are objects emitted by systems, typically when some condition is met. Listeners subscribe to an event type and will receive a callback for each event object emitted. An ``entityx::EventManager`` coordinates subscription and delivery of events between subscribers and emitters. Typically subscribers will be other systems, but need not be.
Events are not part of the original ECS pattern, but they are an efficient alternative to component flags for sending infrequent data.

As an example, we might want to implement a very basic collision system using our ``Position`` data from above.

#### Creating event types

First, we define the event type, which for our example is simply the two entities that collided:

```c++
struct Collision : public Event<Collision> {
  Collision(entityx::Entity left, entityx::Entity right) : left(left), right(right) {}

  entityx::Entity left, right;
};
```

#### Emitting events

Next we implement our collision system, which emits ``Collision`` objects via an ``entityx::EventManager`` instance whenever two entities collide.

```c++
class CollisionSystem : public System<CollisionSystem> {
 public:
  void update(entityx::ptr<entityx::EntityManager> es, entityx::ptr<entityx::EventManager> events, double dt) override {
    entityx::ptr<Position> left_position, right_position;
    for (auto left_entity : es->entities_with_components<Position>(left_position)) {
      for (auto right_entity : es->entities_with_components<Position>(right_position)) {
        if (collide(left_position, right_position)) {
          events->emit<Collision>(left_entity, right_entity);
        }
      }
    }
  };
};
```

#### Subscribing to events

Objects interested in receiving collision information can subscribe to ``Collision`` events by first subclassing the CRTP class ``Receiver<T>``:

```c++
struct DebugSystem : public System<DebugSystem>, Receiver<DebugSystem> {
  void configure(entityx::ptr<entityx::EventManager> event_manager) {
    event_manager->subscribe<Collision>(*this);
  }

  void update(ptr<entityx::EntityManager> entities, ptr<entityx::EventManager> events, double dt) {}

  void receive(const Collision &collision) {
    LOG(DEBUG) << "entities collided: " << collision.left << " and " << collision.right << endl;
  }
};
```

#### Builtin events

Several events are emitted by EntityX itself:

- `EntityCreatedEvent` - emitted when a new entityx::Entity has been created.
  - `entityx::Entity entity` - Newly created entityx::Entity.
- `EntityDestroyedEvent` - emitted when an entityx::Entity is *about to be* destroyed.
  - `entityx::Entity entity` - entityx::Entity about to be destroyed.
- `ComponentAddedEvent<T>` - emitted when a new component is added to an entity.
  - `entityx::Entity entity` - entityx::Entity that component was added to.
  - `entityx::ptr<T> component` - The component added.
- `ComponentRemovedEvent<T>` - emitted when a component is removed from an entity.
  - `entityx::Entity entity` - entityx::Entity that component was removed from.
  - `entityx::ptr<T> component` - The component removed.

#### Implementation notes

- There can be more than one subscriber for an event; each one will be called.
- Event objects are destroyed after delivery, so references should not be retained.
- A single class can receive any number of types of events by implementing a ``receive(const EventType &)`` method for each event type.
- Any class implementing `Receiver` can receive events, but typical usage is to make `System`s also be `Receiver`s.

### Manager (tying it all together)

Managing systems, components and entities can be streamlined by subclassing `Manager`. It is not necessary, but it provides callbacks for configuring systems, initializing entities, and so on.

To use it, subclass `Manager` and implement `configure()`, `initialize()` and `update()`. In this example a new `Manager` is created for each level.

```c++
class Level : public Manager {
public:
  explicit Level(filename string) : filename_(filename) {}

 protected:
  void configure() {
    system_manager->add<DebugSystem>();
    system_manager->add<MovementSystem>();
    system_manager->add<CollisionSystem>();
  };

  void initialize() {
    level_.load(filename_);

    for (auto e : level.entity_data()) {
      entityx::Entity entity = entity_manager->create();
      entity.assign<Position>(rand() % 100, rand() % 100);
      entity.assign<Direction>((rand() % 10) - 5, (rand() % 10) - 5);
    }
  }

  void update(double dt) {
    system_manager->update<MovementSystem>(dt);
    system_manager->update<CollisionSystem>(dt);
  }

  string filename_;
  Level level_;
};
```


Once created, start the manager:

```c++
Level level("mylevel.dat");
level.start();
```

You can then either start the (simplistic) main loop:

```c++
level.run();
```

Or step the entities explicitly inside your own game loop (recommended):

```c++
while (true) {
  level.step(0.1);
}
```

## Installation

EntityX has the following build and runtime requirements:

- A C++ compiler that supports a basic set of C++11 features (ie. Clang >= 3.1, GCC >= 4.7, and maybe (untested) VC++ with the [Nov 2012 CTP](http://www.microsoft.com/en-us/download/details.aspx?id=35515)).
- [CMake](http://cmake.org/)

### C++11 compiler and library support

C++11 support is quite...raw. To make life more interesting, C++ support really means two things: language features supported by the compiler, and library features.

### Installing on OSX Mountain Lion

On OSX you must use Clang as the GCC version is practically prehistoric.

I use Homebrew, and the following works for me:

For libstdc++:

```bash
cmake -DENTITYX_BUILD_SHARED=0 -DENTITYX_BUILD_TESTING=1 ..
```

### Installing on Ubuntu 12.04

On Ubuntu LTS (12.04, Precise) you will need to add some PPAs to get either clang-3.1 or gcc-4.7. Respective versions prior to these do not work.

For gcc-4.7:

```bash
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq
sudo apt-get install gcc-4.7 g++4.7
CC=gcc-4.7 CXX=g++4.7 cmake ...
```

For clang-3.1 (or 3.2 or 3.3):

```bash
sudo apt-add-repository ppa:h-rayflood/llvm
sudo apt-get update -qq
sudo apt-get install clang-3.1
CC=clang-3.1 CXX=clang++3.1 cmake ...
```

### Options

Once these dependencies are installed you should be able to build and install EntityX as below. The following options can be passed to cmake to modify how EntityX is built:

- `-DENTITYX_BUILD_PYTHON=1` - Build Python scripting integration.
- `-DENTITYX_BUILD_TESTING=1` - Build tests (run with `make test`).
- `-DENTITYX_RUN_BENCHMARKS=1` - In conjunction with `-DENTITYX_BUILD_TESTING=1`, also build benchmarks.
- `-DENTITYX_MAX_COMPONENTS=64` - Override the maximum number of components that can be assigned to each entity.
- `-DENTITYX_BUILD_SHARED=1` - Whether to build shared libraries (defaults to 1).
- `-DENTITYX_BUILD_TESTING=0` - Whether to build tests (defaults to 0). Run with "make && make test".

Once you have selected your flags, build and install with:

```sh
mkdir build
cd build
cmake <flags> ..
make
make install
```

EntityX has currently only been tested on Mac OSX (Lion and Mountain Lion), and Linux Debian 12.04. Reports and patches for builds on other platforms are welcome.
