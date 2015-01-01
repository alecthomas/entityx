#!/bin/bash -e

cmake -DCMAKE_BUILD_TYPE=Debug -DENTITYX_BUILD_TESTING=1
make VERBOSE=1
make test || cat Testing/Temporary/LastTest.log
