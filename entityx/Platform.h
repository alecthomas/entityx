#pragma once

#   if defined( __WIN32__ ) || defined( _WIN32 ) || defined( WIN32 ) || defined( _WINDOWS )
#       define DLL_EXPORT __declspec(dllexport)
#       define DLL_IMPORT __declspec(dllimport)
#       if defined ENTITYX_EXPORTS
#           define _entityxExport DLL_EXPORT
#       else
#           define _entityxExport DLL_IMPORT
#       endif
#   else
#       define _entityxExport
#   endif

