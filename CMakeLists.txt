cmake_minimum_required(VERSION 2.8)

set(ENTITYX_MAJOR_VERSION 0)
set(ENTITYX_MINOR_VERSION 8)
set(ENTITYX_PATCH_VERSION 1)
set(ENTITYX_VERSION ${ENTITYX_MAJOR_VERSION}.${ENTITYX_MINOR_VERSION}.${ENTITYX_PATCH_VERSION})


include_directories(${CMAKE_CURRENT_LIST_DIR})

set(ENTITYX_BUILD_TESTING false CACHE BOOL "Enable building of tests.")
set(ENTITYX_RUN_BENCHMARKS false CACHE BOOL "Run benchmarks (in conjunction with -DENTITYX_BUILD_TESTING=1).")
set(ENTITYX_MAX_COMPONENTS 64 CACHE STRING "Set the maximum number of components.")
set(ENTITYX_USE_CPP11_STDLIB true CACHE BOOL "For Clang, specify whether to use libc++ (-stdlib=libc++).")
set(ENTITYX_BUILD_SHARED false CACHE BOOL "Build shared libraries?")
set(ENTITYX_BUILD_PYTHON false CACHE BOOL "Build entityx::python?")

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

add_definitions(-DGTEST_USE_OWN_TR1_TUPLE=1)
set(OLD_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
check_cxx_source_compiles(
"
#include <memory>

int main() {
    std::shared_ptr<int>();
}
"
ENTITYX_HAVE_CXX11_STDLIB
)

if (NOT ENTITYX_HAVE_CXX11_STDLIB)
    message("-- Not using -stdlib=libc++ (test failed to build)")
    set(CMAKE_CXX_FLAGS "${OLD_CMAKE_CXX_FLAGS}")
else ()
    message("-- Using -stdlib=libc++")
endif ()


# Misc features
check_include_file("stdint.h" HAVE_STDINT_H)

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
        ${ARGN}
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

if (ENTITYX_BUILD_PYTHON)
    message("-- Building with Python support (-DENTITYX_BUILD_PYTHON=0 to disable)")
    find_package(Boost 1.48.0 COMPONENTS python)
else (ENTITYX_BUILD_PYTHON)
    message("-- Python support disabled")
endif (ENTITYX_BUILD_PYTHON)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# Things to install
set(install_libs entityx)

set(sources entityx/tags/TagsComponent.cc entityx/deps/Dependencies.cc entityx/System.cc entityx/Event.cc entityx/Entity.cc entityx/Manager.cc entityx/help/Timer.cc)
add_library(entityx STATIC ${sources})

if (ENTITYX_BUILD_SHARED)
    message("-- Building shared libraries (-DENTITYX_BUILD_SHARED=0 to only build static librarires)")
    add_library(entityx_shared SHARED ${sources})
    target_link_libraries(
        entityx_shared
        )
    set_target_properties(entityx_shared PROPERTIES OUTPUT_NAME entityx)
    list(APPEND install_libs entityx_shared)
endif (ENTITYX_BUILD_SHARED)

if (ENTITYX_BUILD_PYTHON AND Boost_PYTHON_LIBRARY)
    message("-- Found boost::python, building entityx/python")
    find_package(PythonLibs REQUIRED)
    include_directories(${PYTHON_INCLUDE_DIRS})
    set(ENTITYX_HAVE_BOOST_PYTHON 1)
    set(python_sources entityx/python/PythonSystem.cc)
    add_library(entityx_python STATIC ${python_sources})
    list(APPEND install_libs entityx_python)
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/entityx/python/entityx/__init__.py
        DESTINATION share/entityx/python/
        RENAME entityx.py
    )
    message("-- Installing entityx Python package to ${CMAKE_INSTALL_PREFIX}/share/entityx/python")
    set(ENTITYX_INSTALLED_PYTHON_PACKAGE_DIR ${CMAKE_INSTALL_PREFIX}/share/entityx/python/)
    if (ENTITYX_BUILD_SHARED)
        add_library(entityx_python_shared SHARED ${python_sources})
        target_link_libraries(
            entityx_python_shared
            entityx_shared
            ${Boost_PYTHON_LIBRARY}
            ${PYTHON_LIBRARIES}
        )
        set_target_properties(entityx_python_shared PROPERTIES OUTPUT_NAME entityx_python)
        list(APPEND install_libs entityx_python_shared)
    endif (ENTITYX_BUILD_SHARED)
    set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS})
    include(CheckNeedGetPointer.cmake)
endif (ENTITYX_BUILD_PYTHON AND Boost_PYTHON_LIBRARY)

if (ENTITYX_BUILD_TESTING)
    #find_package(Boost 1.48.0 REQUIRED COMPONENTS timer system)
    add_subdirectory(gtest-1.6.0)
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
    enable_testing()
    create_test(entity_test entityx/Entity_test.cc)
    create_test(event_test entityx/Event_test.cc)
    create_test(system_test entityx/System_test.cc)
    create_test(tags_component_test entityx/tags/TagsComponent_test.cc)
    create_test(dependencies_test entityx/deps/Dependencies_test.cc)
    if (Boost_PYTHON_LIBRARY)
        add_definitions(-DENTITYX_PYTHON_TEST_DATA=\"${CMAKE_CURRENT_SOURCE_DIR}/entityx/python\")
        create_test(python_test entityx/python/PythonSystem_test.cc entityx_python ${Boost_PYTHON_LIBRARY} ${PYTHON_LIBRARIES})
    endif (Boost_PYTHON_LIBRARY)
    if (ENTITYX_RUN_BENCHMARKS)
        message("-- Running benchmarks")
        add_definitions(-DGTEST_USE_OWN_TR1_TUPLE=1 -DBOOST_NO_CXX11_NUMERIC_LIMITS=1)
        create_test(benchmarks_test entityx/Benchmarks_test.cc)
    else ()
        message("-- Not running benchmarks (use -DENTITYX_RUN_BENCHMARKS=1 to enable)")
    endif ()
endif (ENTITYX_BUILD_TESTING)


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/entityx/config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/entityx/config.h
)

install(
    DIRECTORY "entityx"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.h"
    )

install(
    TARGETS ${install_libs}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
