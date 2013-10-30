#!/bin/bash -e

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Debug -DENTITYX_BUILD_TESTING=1"

if [ "$USE_STD_SHARED_PTR" = "1" ]; then
  CMAKE_ARGS="${CMAKE_ARGS}"
  # This fails on OSX
  if [ "$CXX" = "clang++" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DENTITYX_USE_CPP11_STDLIB=1"
  fi
fi

cmake ${CMAKE_ARGS}
make VERBOSE=1
make test || cat Testing/Temporary/LastTest.log
