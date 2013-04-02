cmake_minimum_required(VERSION 2.8)
project(EntityX)
include_directories(${CMAKE_CURRENT_LIST_DIR})

set(RUN_BENCHMARKS false CACHE BOOL "Run benchmarks")

include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
include(CheckCXXSourceCompiles)

# Default compiler args
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Werror -Wall -Wextra -Wno-unused-parameter -Wno-error=unused-variable -Wno-error=sign-compare -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# C++11 feature checks
include(CheckCXX11Features.cmake)

# Misc features
check_include_file("stdint.h" HAVE_STDINT_H)

set(USE_CPP11_STDLIB false CACHE BOOL "Use the C++11 stdlib (-stdlib=libc++).")

if (USE_CPP11_STDLIB)
    set(OLD_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    check_cxx_source_compiles(
    "
    #include <memory>

    int main() {
        std::shared_ptr<int>();
    }
    "
    HAVE_CXX11_STDLIB
    )

    if (NOT HAVE_CXX11_STDLIB)
        message("-- Not using -stdlib=libc++ (test failed to build)")
        set(CMAKE_CXX_FLAGS "${OLD_CMAKE_CXX_FLAGS}")
    else ()
        message("-- Using -stdlib=libc++")
    endif ()
else ()
    message("-- Using default stdlib (try -DUSE_CPP11_STDLIB=1 to use -stdlib=libc++)")
endif ()

# Check for which shared_ptr implementation to use.
set(USE_STD_SHARED_PTR false CACHE BOOL "Use std::shared_ptr<T> rather than boost::shared_ptr<T>?")

check_cxx_source_compiles(
"
#include <memory>

int main() { std::shared_ptr<int>(); }
"
HAVE_STD_SHARED_PTR
)

check_cxx_source_compiles(
"
#include <boost/shared_ptr.hpp>

int main() { boost::shared_ptr<int>(); }
"
HAVE_BOOST_SHARED_PTR
)

if (HAVE_STD_SHARED_PTR AND USE_STD_SHARED_PTR)
    message("-- Using std::shared_ptr<T>")
else()
    if (USE_STD_SHARED_PTR)
        message("-- Using boost::shared_ptr<T> (std::shared_ptr<T> could not be used)")
    else()
        message("-- Using boost::shared_ptr<T> (try -DUSE_STD_SHARED_PTR=1 to use std::shared_ptr<T>)")
    endif()
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/entityx/config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/entityx/config.h
)

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
        gtest
        gtest_main
        ${Boost_LIBRARIES}
        ${Boost_TIMER_LIBRARY}
        ${Boost_SIGNALS_LIBRARY}
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

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.48.0 REQUIRED COMPONENTS signals)

set(sources entityx/System.cc entityx/Event.cc entityx/Entity.cc entityx/Manager.cc)
add_library(entityx STATIC ${sources})
add_library(entityx_shared SHARED ${sources})
target_link_libraries(
    entityx_shared
    ${Boost_SIGNALS_LIBRARY}
)
set_target_properties(entityx_shared PROPERTIES OUTPUT_NAME entityx)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${GTest_INCLUDE_DIR}
    )

set(BUILD_TESTING false CACHE BOOL "Enable building of tests")
if (BUILD_TESTING)
    find_package(Boost 1.48.0 REQUIRED COMPONENTS signals timer system)
    add_subdirectory(gtest-1.6.0)
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
    enable_testing()
    create_test(entity_test entityx/Entity_test.cc)
    create_test(event_test entityx/Event_test.cc)
    create_test(system_test entityx/System_test.cc)
    create_test(tags_component_test entityx/tags/TagsComponent_test.cc)
    if (RUN_BENCHMARKS)
        message("-- Running benchmarks")
        create_test(benchmarks_test entityx/Benchmarks_test.cc)
    else ()
        message("-- Not running benchmarks (use -DRUN_BENCHMARKS=1 to enable)")
    endif ()
endif (BUILD_TESTING)

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
