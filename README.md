# EntityX - A fast, type-safe C++ Entity Component System
[![Build Status](https://travis-ci.org/alecthomas/entityx.png)](https://travis-ci.org/alecthomas/entityx) [![Build status](https://ci.appveyor.com/api/projects/status/qc8s0pqb5ci092iv/branch/master)](https://ci.appveyor.com/project/alecthomas/entityx/branch/master) [![Gitter chat](https://badges.gitter.im/alecthomas.png)](https://gitter.im/alecthomas/Lobby)


***NOTE: The current stable release 1.0.0 breaks backward compatibility with < 1.0.0. See the [change log](CHANGES.md) for details.***

Entity Component Systems (ECS) are a form of decomposition that completely decouples entity logic and data from the entity "objects" themselves. The [Evolve your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/) article provides a solid overview of EC systems and why you should use them.

EntityX is an EC system that uses C++11 features to provide type-safe component management, event delivery, etc. It was built during the creation of a 2D space shooter.

## Downloading

You can acquire stable releases [here](https://github.com/alecthomas/entityx/releases).

Alternatively, you can check out the current development version with:

```
git clone https://github.com/alecthomas/entityx.git
```

See [below](#installation) for installation instructions.

## Contact

Feel free to jump on my [Gitter channel](https://gitter.im/alecthomas/Lobby) if you have any questions/comments. This is a single channel for all of my projects, so please mention you're asking about EntityX to avoid (my) confusion.

You can also contact me directly via [email](mailto:alec@swapoff.org) or [Twitter](https://twitter.com/alecthomas).

## Benchmarks / Comparisons

EntityX includes its own benchmarks, but @abeimler has created [a benchmark suite](https://github.com/abeimler/ecs_benchmark/blob/master/doc/BenchmarkResultDetails2.md) testing up to 2M entities in EntityX, the EntityX compile-time branch, Anax, and Artemis C++.

## Recent Notable Changes

- 2014-03-02 - (1.0.0alpha1) Switch to using cache friendly component storage (big breaking change). Also eradicated use of `std::shared_ptr` for components.
- 2014-02-13 - Visual C++ support thanks to [Jarrett Chisholm](https://github.com/jarrettchisholm)!
- 2013-10-29 - Boost has been removed as a primary dependency for builds not using python.
- 2013-08-21 - Remove dependency on `boost::signal` and switch to embedded [Simple::Signal](http://timj.testbit.eu/2013/cpp11-signal-system-performance/).
- 2013-08-18 - Destroying an entity invalidates all other references
- 2013-08-17 - Python scripting, and a more robust build system

See the [ChangeLog](https://github.com/alecthomas/entityx/blob/master/CHANGES.md) for details.

## EntityX extensions and example applications

- [Will Usher](https://github.com/Twinklebear) has also written an [Asteroids clone](https://github.com/Twinklebear/asteroids).
- [Roc Solid Productions](https://github.com/RocSolidProductions) have written a [space shooter](https://github.com/RocSolidProductions/Space-Shooter)!
- Giovani Milanez's first [game](https://github.com/giovani-milanez/SpaceTD).
- [A game](https://github.com/ggc87/BattleCity2014) using Ogre3D and EntityX.

**DEPRECATED - 0.1.x ONLY**

- [Wu Zhenwei](https://github.com/acaly) has written [Lua bindings](https://github.com/acaly/entityx_lua) for EntityX, allowing entity logic to be extended through Lua scripts.
- [Python bindings](https://github.com/alecthomas/entityx_python) allowing entity logic to be extended through Python scripts.
- [Rodrigo Setti](https://github.com/rodrigosetti) has written an OpenGL [Asteroids clone](https://github.com/rodrigosetti/azteroids) which uses EntityX.

## Example

An SFML2 example application is [available](/examples/example.cc) that shows most of EntityX's concepts. It spawns random circles on a 2D plane moving in random directions. If two circles collide they will explode and emit particles. All circles and particles are entities.

It illustrates:

- Separation of data via components.
- Separation of logic via systems.
- Use of events (colliding bodies trigger a CollisionEvent).

Compile with:

    c++ -O3 -std=c++11 -Wall -lsfml-system -lsfml-window -lsfml-graphics -lentityx example.cc -o example

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
#include <entityx/entityx.h>

entityx::EntityX ex;

entityx::Entity entity = ex.entities.create();
```

And destroying an entity is done with:

```c++
entity.destroy();
```

#### Implementation details

- Each `entityx::Entity` is a convenience class wrapping an `entityx::Entity::Id`.
- An `entityx::Entity` handle can be invalidated with `invalidate()`. This does not affect the underlying entity.
- When an entity is destroyed the manager adds its ID to a free list and invalidates the `entityx::Entity` handle.
- When an entity is created IDs are recycled from the free list first, before allocating new ones.
- An `entityx::Entity` ID contains an index and a version. When an entity is destroyed, the version associated with the index is incremented, invalidating all previous entities referencing the previous ID.
- To improve cache coherence, components are constructed in contiguous memory ranges by using `entityx::EntityManager::assign<C>(id, ...)`.

### Components (entity data)

The general idea with the EntityX interpretation of ECS is to have as little logic in components as possible. All logic should be contained in Systems.

To that end Components are typically [POD types](http://en.wikipedia.org/wiki/Plain_Old_Data_Structures) consisting of self-contained sets of related data. Components can be any user defined struct/class.

#### Creating components

As an example, position and direction information might be represented as:

```c++
struct Position {
  Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  float x, y;
};

struct Direction {
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

#### Querying entities and their components

To query all entities with a set of components assigned you can use two
methods. Both methods will return only those entities that have *all* of the
specified components associated with them.

`entityx::EntityManager::each(f)` provides functional-style iteration over
entity components:

```c++
entities.each<Position, Direction>([](Entity entity, Position &position, Direction &direction) {
  // Do things with entity, position and direction.
});
```


For iterator-style traversal of components, use
``entityx::EntityManager::entities_with_components()``:

```c++
ComponentHandle<Position> position;
ComponentHandle<Direction> direction;
for (Entity entity : entities.entities_with_components(position, direction)) {
  // Do things with entity, position and direction.
}
```

To retrieve a component associated with an entity use ``entityx::Entity::component<C>()``:

```c++
ComponentHandle<Position> position = entity.component<Position>();
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
- Each type of component is allocated in (mostly) contiguous blocks to improve cache coherency.

### Systems (implementing behavior)

Systems implement behavior using one or more components. Implementations are subclasses of `System<T>` and *must* implement the `update()` method, as shown below.

A basic movement system might be implemented with something like the following:

```c++
struct MovementSystem : public System<MovementSystem> {
  void update(entityx::EntityManager &es, entityx::EventManager &events, TimeDelta dt) override {
    es.each<Position, Direction>([dt](Entity entity, Position &position, Direction &direction) {
      position.x += direction.x * dt;
      position.y += direction.y * dt;
    });
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
struct Collision {
  Collision(entityx::Entity left, entityx::Entity right) : left(left), right(right) {}

  entityx::Entity left, right;
};
```

#### Emitting events

Next we implement our collision system, which emits ``Collision`` objects via an ``entityx::EventManager`` instance whenever two entities collide.

```c++
class CollisionSystem : public System<CollisionSystem> {
 public:
  void update(entityx::EntityManager &es, entityx::EventManager &events, TimeDelta dt) override {
    ComponentHandle<Position> left_position, right_position;
    for (Entity left_entity : es.entities_with_components(left_position)) {
      for (Entity right_entity : es.entities_with_components(right_position)) {
        if (collide(left_position, right_position)) {
          events.emit<Collision>(left_entity, right_entity);
        }
      }
    }
  };
};
```

#### Subscribing to events

Objects interested in receiving collision information can subscribe to ``Collision`` events by first subclassing the CRTP class ``Receiver<T>``:

```c++
struct DebugSystem : public System<DebugSystem>, public Receiver<DebugSystem> {
  void configure(entityx::EventManager &event_manager) {
    event_manager.subscribe<Collision>(*this);
  }

  void update(entityx::EntityManager &entities, entityx::EventManager &events, TimeDelta dt) {}

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
- `ComponentAddedEvent<C>` - emitted when a new component is added to an entity.
  - `entityx::Entity entity` - entityx::Entity that component was added to.
  - `ComponentHandle<C> component` - The component added.
- `ComponentRemovedEvent<C>` - emitted when a component is removed from an entity.
  - `entityx::Entity entity` - entityx::Entity that component was removed from.
  - `ComponentHandle<C> component` - The component removed.

#### Implementation notes

- There can be more than one subscriber for an event; each one will be called.
- Event objects are destroyed after delivery, so references should not be retained.
- A single class can receive any number of types of events by implementing a ``receive(const EventType &)`` method for each event type.
- Any class implementing `Receiver` can receive events, but typical usage is to make `System`s also be `Receiver`s.
- When an `Entity` is destroyed it will cause all of its components to be removed. This triggers `ComponentRemovedEvent`s to be triggered for each of its components. These events are triggered before the `EntityDestroyedEvent`.

### Manager (tying it all together)

Managing systems, components and entities can be streamlined by using the
"quick start" class `EntityX`. It simply provides pre-initialized
`EventManager`, `EntityManager` and `SystemManager` instances.

To use it, subclass `EntityX`:

```c++
class Level : public EntityX {
public:
  explicit Level(filename string) {
    systems.add<DebugSystem>();
    systems.add<MovementSystem>();
    systems.add<CollisionSystem>();
    systems.configure();

    level.load(filename);

    for (auto e : level.entity_data()) {
      entityx::Entity entity = entities.create();
      entity.assign<Position>(rand() % 100, rand() % 100);
      entity.assign<Direction>((rand() % 10) - 5, (rand() % 10) - 5);
    }
  }

  void update(TimeDelta dt) {
    systems.update<DebugSystem>(dt);
    systems.update<MovementSystem>(dt);
    systems.update<CollisionSystem>(dt);
  }

  Level level;
};
```


You can then step the entities explicitly inside your own game loop:

```c++
while (true) {
  level.update(0.1);
}
```

## Installation

### Arch Linux

    pacman -S entityx

### OSX

    brew install entityx

### Windows

Build it manually.

Requirements:

* [Visual Studio 2015](https://www.visualstudio.com/en-us/downloads/visual-studio-2015-downloads-vs.aspx) or later, or a C++ compiler that supports a basic set of C++11 features (ie. Clang >= 3.1 or GCC >= 4.7).
* [CMake](http://cmake.org/)

### Building entityx - Using vcpkg

You can download and install entityx using the [vcpkg](https://github.com/Microsoft/vcpkg) dependency manager:

    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    ./vcpkg integrate install
    ./vcpkg install entityx

The entityx port in vcpkg is kept up to date by Microsoft team members and community contributors. If the version is out of date, please [create an issue or pull request](https://github.com/Microsoft/vcpkg) on the vcpkg repository.

### Other systems

Build it manually.

Requirements:

* A C++ compiler that supports a basic set of C++11 features (ie. Clang >= 3.1, GCC >= 4.7).
* [CMake](http://cmake.org/)

### C++11 compiler and library support

C++11 support is quite...raw. To make life more interesting, C++ support really means two things: language features supported by the compiler, and library features. EntityX tries to support the most common options, including the default C++ library for the compiler/platform, and libstdc++.

### Installing on Ubuntu 12.04

On Ubuntu LTS (12.04, Precise) you will need to add some PPAs to get either clang-3.1 or gcc-4.7. Respective versions prior to these do not work.

For gcc-4.7:

```bash
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq
sudo apt-get install gcc-4.7 g++-4.7
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

- `-DENTITYX_RUN_BENCHMARKS=1` - In conjunction with `-DENTITYX_BUILD_TESTING=1`, also build benchmarks.
- `-DENTITYX_MAX_COMPONENTS=64` - Override the maximum number of components that can be assigned to each entity.
- `-DENTITYX_BUILD_SHARED=1` - Whether to build shared libraries (defaults to 1).
- `-DENTITYX_BUILD_TESTING=1` - Whether to build tests (defaults to 0). Run with "make && make test".
- `-DENTITYX_DT_TYPE=double` - The type used for delta time in EntityX update methods.

Once you have selected your flags, build and install with:

```sh
mkdir build
cd build
cmake <flags> ..
make
make install
```

EntityX has currently only been tested on Mac OSX (Lion and Mountain Lion), Linux Debian 12.04 and Arch Linux. Reports and patches for builds on other platforms are welcome.
