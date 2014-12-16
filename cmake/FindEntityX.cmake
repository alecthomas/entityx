# Look for a version of EntityX on the local machine
#
# By default, this will look in all common places. If EntityX is built or
# installed in a custom location, you're able to either modify the
# CMakeCache.txt file yourself or simply pass the path to CMake using either the
# environment variable `ENTITYX_ROOT` or the CMake define with the same name.

set(ENTITYX_PATHS	${ENTITYX_ROOT}
					$ENV{ENTITYX_ROOT}
					~/Library/Frameworks
					/Library/Frameworks
					/usr/local
					/usr
					/sw
					/opt/local
					/opt/csw
					/opt)

find_path(ENTITYX_INCLUDE_DIR entityx/entityx.h PATH_SUFFIXES include PATHS ${ENTITYX_PATHS})
find_library(ENTITYX_LIBRARY NAMES entityx PATH_SUFFIXES lib PATHS ${ENTITYX_PATHS})
find_library(ENTITYX_LIBRARY_DEBUG NAMES entityx-d PATH_SUFFIXES lib PATHS ${ENTITYX_PATHS})
mark_as_advanced(ENTITYX_INCLUDE_DIR ENTITYX_LIBRARY)

if(ENTITYX_INCLUDE_DIR AND ENTITYX_LIBRARY)
	set(ENTITYX_FOUND TRUE)
else()
	set(ENTITYX_FOUND FALSE)
endif()
