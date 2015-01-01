project(EntityX)
cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

set(ENTITYX_MAJOR_VERSION 2)
set(ENTITYX_MINOR_VERSION 0)
set(ENTITYX_PATCH_VERSION 0)
set(ENTITYX_VERSION ${ENTITYX_MAJOR_VERSION}.${ENTITYX_MINOR_VERSION}.${ENTITYX_PATCH_VERSION})


include_directories(${CMAKE_CURRENT_LIST_DIR})

set(ENTITYX_BUILD_TESTING false CACHE BOOL "Enable building of tests.")
set(ENTITYX_RUN_BENCHMARKS false CACHE BOOL "Run benchmarks (in conjunction with -DENTITYX_BUILD_TESTING=1).")

include(CheckCXXSourceCompiles)

# Default compiler args
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "(GNU|.*Clang)")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Werror -Wall -Wextra -Wno-unused-parameter -Wno-error=unused-variable -Wno-error=sign-compare -std=c++11")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "-g -Os -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "-g -O2 -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	# /Zi - Produces a program database (PDB) that contains type information and symbolic debugging information for use with the debugger.
	# /FS - Allows multiple cl.exe processes to write to the same .pdb file
	# /DEBUG - Enable debug during linking
	# /Od - Disables optimization
	set(CMAKE_CXX_FLAGS_DEBUG "/Zi /FS /DEBUG /Od /MDd")
	# /Ox - Full optimization
	set(CMAKE_CXX_FLAGS_RELEASE "/Ox -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Ox /Zi /FS /DEBUG")
endif()

# if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -Wno-c++98-compat -Wno-shadow -Wno-padded -Wno-missing-noreturn -Wno-global-constructors")
# endif()

# Library installation directory
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)
set(libdir ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

set(OLD_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
if (("${CMAKE_CXX_COMPILER_ID}" MATCHES "(GNU|.*Clang)"))
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
    message("-- Not using -stdlib=libc++ (test failed to build)")
    set(CMAKE_CXX_FLAGS "${OLD_CMAKE_CXX_FLAGS}")
else ()
    message("-- Using -stdlib=libc++")
endif ()


macro(require FEATURE_NAME MESSAGE_STRING)
    if (NOT ${${FEATURE_NAME}})
        message(FATAL_ERROR "${MESSAGE_STRING} required -- ${${FEATURE_NAME}}")
    else()
        message("--   ${MESSAGE_STRING} found")
    endif()
endmacro(require)

macro(create_test TARGET_NAME SOURCE)
    add_executable(${TARGET_NAME} ${SOURCE})
    add_test(${TARGET_NAME} ${TARGET_NAME})
endmacro()

if (NOT CMAKE_BUILD_TYPE)
    message("-- Defaulting to release build (use -DCMAKE_BUILD_TYPE:STRING=Debug for debug build)")
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Things to install
set(install_libs entityx)

set(sources entityx/entityx.hh)

if (ENTITYX_BUILD_TESTING)
    enable_testing()
    create_test(entity_test tests/entity_test.cc)
    if (ENTITYX_RUN_BENCHMARKS)
        message("-- Building benchmarks")
        create_test(benchmarks_test tests/benchmarks_test.cc)
    else ()
        message("-- Not building benchmarks (use -DENTITYX_RUN_BENCHMARKS=1 to enable)")
    endif ()
else ()
    message("-- Not building tests (use -DENTITYX_BUILD_TESTING=1 to enable)")
endif (ENTITYX_BUILD_TESTING)


if (NOT WINDOWS OR CYGWIN)
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
    FILES_MATCHING PATTERN "*.hh"
    )
