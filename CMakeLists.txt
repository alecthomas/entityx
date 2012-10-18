cmake_minimum_required(VERSION 2.8)
project(EntityX)
include_directories(. ./c++11)

# C++11 feature checks
include(CheckCXX11Features.cmake)

MACRO(REQUIRE FEATURE_NAME MESSAGE_STRING)
    if (NOT DEFINED ${FEATURE_NAME})
        message(FATAL_ERROR ${MESSAGE_STRING})
    endif()
ENDMACRO(REQUIRE)

require(HAS_CXX11_AUTO "C++11 auto support is required")
require(HAS_CXX11_NULLPTR "C++11 nullptr support is required")
require(HAS_CXX11_RVALUE_REFERENCES "C++11 rvalue reference support is required")
require(HAS_CXX11_CSTDINT_H "C++11 stdint support is required")
require(HAS_CXX11_VARIADIC_TEMPLATES "C++11 variadic templates required")

enable_testing()
find_package(GTest REQUIRED)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
#find_package(Boost REQUIRED COMPONENTS)
find_package(Boost 1.36.0 REQUIRED COMPONENTS signals)

set(CMAKE_CXX_FLAGS "-ansi -pedantic -Werror -Wall -Wextra -Wno-unused-parameter -Wno-error=unused-variable -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")


include_directories(
    ${Boost_INCLUDE_DIR}
    ${GTest_INCLUDE_DIR}
    )
add_executable(
    entityx_test
    entityx/Components_test.cc
    entityx/Entity_test.cc
    entityx/Event_test.cc
    entityx/System_test.cc
    )
target_link_libraries(
    entityx_test
    entityx
    glog
    ${Boost_LIBRARIES}
    ${GTEST_BOTH_LIBRARIES}
    )
add_test(AllTestsInentityx_test entityx_test)
add_library(
    entityx
    entityx/Components.cc
    entityx/System.cc
    entityx/Event.cc
    entityx/Entity.cc
    entityx/World.cc
    )
