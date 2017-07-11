# Change Log

## 2014-03-02 - 1.0.0alpha1 - Cache coherence + breaking changes

EntityX has switched to a more cache-friendly memory layout for components. This is achieved by requiring the use of `assign<Component>(arg0, arg1, ...)` and removing `assign(component)`. This allows EntityX to explicitly control the layout of components. The current layout algorithm reserves space for components in chunks (8192 by default).

This change also necessitated a move away from the use of `shared_ptr<>` for components, which I had never been that pleased with anyway. Replacing it is a very lightweight `ComponentHandle<C>` smart pointer. This checks for validity of the entity associated with the component, and validity of the component itself. It also allows future iterations of EntityX to do even more interesting things with memory layout if desirable.


## 2014-02-13 - Support for Visual C++

[Jarrett Chisholm](https://github.com/jarrettchisholm) has added conditional compilation support for VC++ and fixed some issues that prevented compilation, so EntityX now fully supports Visual C++!

You will need at least [Visual Studio 2013](http://www.microsoft.com/en-ca/download/details.aspx?id=40787) with [Update 1](http://www.microsoft.com/en-us/download/details.aspx?id=41650) and [Update 2 CTP](http://www.microsoft.com/en-us/download/details.aspx?id=41699) installed. The usual CMake installation instructions should "just work" and correctly provide VC++ support.

## 2013-10-29 [no-boost branch] - Removed boost dependency for everything except python integration.

This branch requires C++11 support and has removed all the non-boost::python dependecies, reducing the overhead of running entityx.

## 2013-08-22 - Remove `boost::signal` and switch to `Simple::Signal`.

According to the [benchmarks](http://timj.testbit.eu/2013/cpp11-signal-system-performance/) Simple::Signal is an order of magnitude faster than `boost::signal`. Additionally, `boost::signal` is now deprecated in favor of `boost::signal2`, which is not supported on versions of Boost on a number of platforms.

This is an implementation detail and should not affect EntityX users at all.

## 2013-08-18 - Destroying an entity invalidates all other references

Previously, `Entity::Id` was a simple integer index (slot) into vectors in the `EntityManager`. EntityX also maintains a list of deleted entity slots that are reused when new entities are created. This reduces the size and frequency of vector reallocation. The downside though, was that if a slot was reused, entity IDs referencing the entity before reallocation would be invalidated on reuse.

Each slot now also has a version number and a "valid" bit associated with it. When an entity is allocated the version is incremented and the valid bit set. When an entity is destroyed, the valid bit is cleared. `Entity::Id` now contains all of this information and can correctly determine if an ID is still valid across destroy/create.

## 2013-08-17 - Python scripting, and a more robust build system

Two big changes in this release:

1. Python scripting support (alpha).
    - Bridges the EntityX entity-component system into Python.
    - Components and entities can both be defined in Python.
    - Systems must still be defined in C++, for performance reasons.

    Note that there is one major design difference between the Python ECS model and the C++ model: entities in Python can receive and handle events.
 
    See the [README](https://github.com/alecthomas/entityx/blob/master/entityx/python/README.md) for help, and the [C++](https://github.com/alecthomas/entityx/blob/master/entityx/python/PythonSystem_test.cc) and [Python](https://github.com/alecthomas/entityx/tree/master/entityx/python/entityx/tests) test source for more examples.

2. Made the build system much more robust, including automatic feature selection with manual override.
