#ifndef __Platform
#define __Platform

// ===================================================================
// Disable warnings
// ===================================================================
// MSVC++ Pragmas
#pragma warning(disable : 4995) // OLD_IOSTREAMS DEPRECATED
#pragma warning(disable : 4290) // throw specification warning (not yet implemented in visual C++)
#pragma warning(disable : 4786) // STL expansion too long
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#pragma warning(disable : 4996)

#define PLATFORM_WIN32 1
#define PLATFORM_MACOS 2
#define PLATFORM_LINUX 3

// Platform
#   if defined( __WIN32__ ) || defined( _WIN32 ) || defined( WIN32 ) || defined( _WINDOWS )
#       define FIREFLY_PLATFORM PLATFORM_WIN32
#       define WIN32_LEAN_AND_MEAN // prevent windows.h from including winsock.h
#       include <Windows.h>
#       include <process.h>
#       include <conio.h>
#       include <objbase.h>
#       include <time.h>
#       define DLL_EXPORT __declspec(dllexport)
#       define DLL_IMPORT __declspec(dllimport)
#       if defined FIREFLY_EXPORTS
#           define _fireflyExport DLL_EXPORT
#       else
#           define _fireflyExport DLL_IMPORT
#       endif
        struct HINSTANCE__;
        typedef struct HINSTANCE__* hInstance;
#       define HMODULE hInstance
#       define DeviceHandle HWND
#       define WindowHandle HWND
#       define DLL_LOAD( a ) LoadLibrary( a )
#       define DLL_GETPROCADDR( a, b ) GetProcAddress( a, b )
#       define DLL_FREE( a ) FreeLibrary( a )
#       define DLL_EXT "dll"
#       define DLL_ERROR() ""
#       define DLL_PREFIX ""
#       define PATH_SEPERATOR "\\"
#   else
#       define MAX_PATH 255
#       define PATH_SEPERATOR "/"
#       if defined( __APPLE__)        
#           define FIREFLY_PLATFORM PLATFORM_MACOS
#           include <dlfcn.h>
#           include <pthread.h>
#           include <unistd.h>
#           include <dirent.h>
#           include <stdlib.h>
#           include <sys/time.h>
#           define __stdcall
#           define DLL_EXPORT
#           define DLL_IMPORT
#           define _fireflyExport
#           define HMODULE void*
#           define DeviceHandle long
#           define WindowHandle void*
#           define DLL_LOAD( a ) dlopen( a , RTLD_LAZY)
#           define DLL_GETPROCADDR( a, b ) dlsym( a, b )
#           define DLL_FREE( a ) dlclose( a )
#           define DLL_EXT "dylib"
#           define DLL_ERROR() dlerror()
#           define DLL_PREFIX "lib"
#       else
#           if defined(__linux__)
#               define FIREFLY_PLATFORM PLATFORM_LINUX
#               include <dlfcn.h>
#               include <pthread.h>
#               include <unistd.h>
#               include <dirent.h>
#               include <sys/time.h>
#               define __stdcall
#               define DLL_EXPORT
#               define DLL_IMPORT
#               define _fireflyExport
#               define HMODULE void*
#               define DeviceHandle void*
#               define WindowHandle void*
#               define DLL_LOAD( a ) dlopen( a , RTLD_LAZY)
#               define DLL_GETPROCADDR( a, b ) dlsym( a, b )
#               define DLL_FREE( a ) dlclose( a )
#               define DLL_EXT "so"
#               define DLL_ERROR() dlerror()
#               define DLL_PREFIX "lib"
#           endif
#       endif
#   endif

#define BIT_SET(a,b) (a |= b)
#define BIT_CLEAR(a,b)(a &= ~b)
#define BIT_FLIP(a,b)(a ^= b)
#define BIT_CHECK(a,b)(a & b)

#endif
