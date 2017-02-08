cmake_minimum_required(VERSION 3.1)
set(ENTITYX_MAJOR_VERSION 1)
set(ENTITYX_MINOR_VERSION 1)
set(ENTITYX_PATCH_VERSION 2)
set(ENTITYX_VERSION ${ENTITYX_MAJOR_VERSION}.${ENTITYX_MINOR_VERSION}.${ENTITYX_PATCH_VERSION})
set(CMAKE_CXX_EXTENSIONS OFF)

project(EntityX)

message(STATUS "EntityX version ${ENTITYX_VERSION}")

if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()


include_directories(${CMAKE_CURRENT_LIST_DIR})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ENTITYX_BUILD_TESTING true CACHE BOOL "Enable building of tests.")
set(ENTITYX_RUN_BENCHMARKS false CACHE BOOL "Run benchmarks (in conjunction with -DENTITYX_BUILD_TESTING=1).")
set(ENTITYX_MAX_COMPONENTS 64 CACHE STRING "Set the maximum number of components.")
set(ENTITYX_DT_TYPE double CACHE STRING "The type used for delta time in EntityX update methods.")
set(ENTITYX_BUILD_SHARED true CACHE BOOL "Build shared libraries?")

include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
include(CheckCXXSourceCompiles)

# Default compiler args
if (CMAKE_CXX_COMPILER_ID MATCHES "(GNU|.*Clang)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Werror -Wall -Wextra -Wno-unused-parameter -Wno-error=unused-variable -Wno-error=sign-compare")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-g -Os -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-g -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL 'MSVC')
    # /Zi - Produces a program database (PDB) that contains type information and symbolic debugging information for use with the debugger.
    # /FS - Allows multiple cl.exe processes to write to the same .pdb file
    # /DEBUG - Enable debug during linking
    # /Od - Disables optimization
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /FS /DEBUG /Od /MDd")
    # /Ox - Full optimization
    set(CMAKE_CXX_FLAGS_RELEASE "/Ox -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Ox /Zi /FS /DEBUG")
endif()

# if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -Wno-c++98-compat -Wno-shadow -Wno-padded -Wno-missing-noreturn -Wno-global-constructors")
# endif()

# Library installation directory
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)
set(libdir ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

set(OLD_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if ((MAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
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
    message(STATUS "-- Not using -stdlib=libc++ (test failed to build)")
    set(CMAKE_CXX_FLAGS "${OLD_CMAKE_CXX_FLAGS}")
else ()
    message(STATUS "-- Using -stdlib=libc++")
endif ()


# Misc features
check_include_file("stdint.h" HAVE_STDINT_H)

macro(require FEATURE_NAME MESSAGE_STRING)
    if (NOT ${${FEATURE_NAME}})
        message(FATAL_ERROR "${MESSAGE_STRING} required -- ${${FEATURE_NAME}}")
    else()
        message(STATUS "--   ${MESSAGE_STRING} found")
    endif()
endmacro(require)

macro(create_test TARGET_NAME SOURCE DEPENDENCIES)
    add_executable(${TARGET_NAME} ${SOURCE})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "entityx/tests")
    target_link_libraries(
        ${TARGET_NAME}
        ${DEPENDENCIES}
        ${ARGN}
        )

    # Special case for benchmark tests
    if (${TARGET_NAME} MATCHES .*benchmark.*)
        if(ENTITYX_RUN_BENCHMARKS)
            add_test(${TARGET_NAME} ${TARGET_NAME})
        endif()
    else()
        add_test(${TARGET_NAME} ${TARGET_NAME})
    endif()
endmacro()

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "-- Defaulting to release build (use -DCMAKE_BUILD_TYPE:STRING=Debug for debug build)")
    set(CMAKE_BUILD_TYPE "Release")
endif()

message(STATUS "-- Checking misc features")
require(HAVE_STDINT_H "stdint.h")

# Things to install


set(sources entityx/System.cc entityx/Event.cc entityx/Entity.cc entityx/help/Timer.cc entityx/help/Pool.cc)

if (ENTITYX_BUILD_SHARED)
    message(STATUS "-- Building shared libraries (-DENTITYX_BUILD_SHARED=0 to only build static libraries)")
    add_library(entityx SHARED ${sources})

    set_target_properties(entityx PROPERTIES
        OUTPUT_NAME entityx
        DEBUG_POSTFIX -d
        VERSION ${ENTITYX_VERSION}
        SOVERSION ${ENTITYX_MAJOR_VERSION}
        FOLDER entityx)
    set(install_libs entityx)
    set_property(TARGET entityx APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
else()
    add_library(entityx STATIC ${sources})
    set_target_properties(entityx PROPERTIES DEBUG_POSTFIX -d FOLDER entityx)
    set(install_libs entityx)
    set_property(TARGET entityx APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
endif (ENTITYX_BUILD_SHARED)

# Make sure C++11 features are available
target_compile_features(entityx PUBLIC cxx_auto_type)
target_compile_features(entityx PUBLIC cxx_nullptr)
target_compile_features(entityx PUBLIC cxx_static_assert)
target_compile_features(entityx PUBLIC cxx_decltype)
target_compile_features(entityx PUBLIC cxx_constexpr)
target_compile_features(entityx PUBLIC cxx_sizeof_member)
target_compile_features(entityx PUBLIC cxx_variadic_templates)
target_compile_features(entityx PUBLIC cxx_rvalue_references)
target_compile_features(entityx PUBLIC cxx_long_long_type)
target_compile_features(entityx PUBLIC cxx_lambdas)
target_compile_features(entityx PUBLIC cxx_func_identifier)

if (ENTITYX_BUILD_TESTING)
    enable_testing()
    create_test(pool_test entityx/help/Pool_test.cc ${install_libs})
    create_test(entity_test entityx/Entity_test.cc ${install_libs})
    create_test(event_test entityx/Event_test.cc ${install_libs})
    create_test(system_test entityx/System_test.cc ${install_libs})
    create_test(tags_component_test entityx/tags/TagsComponent_test.cc ${install_libs})
    create_test(dependencies_test entityx/deps/Dependencies_test.cc ${install_libs})
    create_test(benchmarks_test entityx/Benchmarks_test.cc ${install_libs})
    if (ENTITYX_RUN_BENCHMARKS)
        message(STATUS "-- Running benchmarks")
    else ()
        message(STATUS "-- Not running benchmarks (use -DENTITYX_RUN_BENCHMARKS=1 to enable)")
    endif ()
endif (ENTITYX_BUILD_TESTING)


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/entityx/config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/entityx/config.h
)


if (NOT WINDOWS OR CYGWIN)
    set(entityx_libs -lentityx)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/entityx.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/entityx.pc
        )

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/entityx.pc
        DESTINATION "${libdir}/pkgconfig"
        )
endif()

install(
    DIRECTORY "entityx"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.h"
    )

install(
    TARGETS ${install_libs}
    LIBRARY DESTINATION "${libdir}"
    ARCHIVE DESTINATION "${libdir}"
    RUNTIME DESTINATION "bin"
    )
