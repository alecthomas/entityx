cmake_minimum_required(VERSION 2.8)
project(EntityX)
include_directories(.)
enable_testing()
find_package(GTest REQUIRED)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
#find_package(Boost REQUIRED COMPONENTS)
find_package(Boost 1.36.0 REQUIRED COMPONENTS signals)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${GTest_INCLUDE_DIR}
    )
add_definitions(
    -ansi -pedantic -Werror -Wall -Wextra -Wno-unused-parameter -Wno-error=unused-variable -std=c++11
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
