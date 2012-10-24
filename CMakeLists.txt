cmake_minimum_required(VERSION 2.8)
project(EntityX)
include_directories(${CMAKE_CURRENT_LIST_DIR})

include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
# C++11 feature checks
include(CheckCXX11Features.cmake)
# Misc features
CHECK_INCLUDE_FILE("stdint.h" HAVE_STDINT_H)

macro(require FEATURE_NAME MESSAGE_STRING)
    if (NOT ${${FEATURE_NAME}})
        message(FATAL_ERROR "${MESSAGE_STRING} required -- ${${FEATURE_NAME}}")
    else()
        message("--   ${MESSAGE_STRING} found")
    endif()
endmacro(require)

macro(create_test TARGET_NAME SOURCE)
    add_executable(${TARGET_NAME} ${SOURCE})
    target_link_libraries(
        ${TARGET_NAME}
        entityx
        glog
        ${Boost_LIBRARIES}
        ${GTEST_BOTH_LIBRARIES}
        )
    add_test(${TARGET_NAME} ${TARGET_NAME})
endmacro()

if (NOT CMAKE_BUILD_TYPE)
    message("-- Defaulting to release build (use -DCMAKE_BUILD_TYPE:STRING=Debug for debug build)")
    set(CMAKE_BUILD_TYPE "Release")
endif()

message("-- Checking C++ features")
require(HAS_CXX11_AUTO "C++11 auto support")
require(HAS_CXX11_NULLPTR "C++11 nullptr support")
require(HAS_CXX11_RVALUE_REFERENCES "C++11 rvalue reference support")
#require(HAS_CXX11_CSTDINT_H "C++11 stdint support")
require(HAS_CXX11_VARIADIC_TEMPLATES "C++11 variadic templates")
require(HAS_CXX11_RVALUE_REFERENCES "C++11 rvalue references")
require(HAS_CXX11_LONG_LONG "C++11 long long")
require(HAS_CXX11_LONG_LONG "C++11 lambdas")

message("-- Checking misc features")
require(HAVE_STDINT_H "stdint.h")

enable_testing()
find_package(GTest REQUIRED)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.48.0 REQUIRED COMPONENTS signals)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Werror -Wall -Wextra -Wno-unused-parameter -Wno-error=unused-variable -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

set(sources entityx/Components.cc entityx/System.cc entityx/Event.cc entityx/Entity.cc entityx/Manager.cc)
add_library(entityx STATIC ${sources})
add_library(entityx_shared SHARED ${sources})

include_directories(
    ${Boost_INCLUDE_DIR}
    ${GTest_INCLUDE_DIR}
    )

create_test(entity_test entityx/Entity_test.cc)
create_test(component_test entityx/Components_test.cc)
create_test(event_test entityx/Event_test.cc)
create_test(system_test entityx/System_test.cc)

file(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/entityx/*.h")

install(
    FILES ${headers}
    DESTINATION "include/entityx"
    )

install(
    TARGETS entityx entityx_shared
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
