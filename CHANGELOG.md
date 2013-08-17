# Change Log

## 2013-08-17 - Python scripting, and a more robust build system

Two big changes in this release:

1. Python scripting support (alpha).

    - Bridges the EntityX entity-component system into Python.
    - Components and entities can both be defined in Python.
    - Systems must still be defined in C++, for performance reasons.

    Note that there is one major design difference between the Python ECS model and the C++ model: entities in Python can receive and handle events.
 
    See the [README](https://github.com/alecthomas/entityx/blob/master/entityx/python/README.md) for help, and the [C++](https://github.com/alecthomas/entityx/blob/master/entityx/python/PythonSystem_test.cc) and [Python](https://github.com/alecthomas/entityx/tree/master/entityx/python/entityx/tests) test source for more examples.

2. Updated the build system to 
