# FindSteamworksSDK.cmake
#
# This module looks for the Steamworks SDK and creates an imported target.
# It expects `Steamworks_ROOT` to be set to the base directory of the SDK.

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_steam_bitness 64)
else ()
    set(_steam_bitness 32)
endif ()

# Setup library names and binary directories based on platform
set(_steam_lib_name "steam_api")
set(_steam_bin_dir "redistributable_bin")

if (WIN32)
    if (_steam_bitness EQUAL 64)
        set(_steam_lib_name "steam_api64")
        set(_steam_bin_dir "redistributable_bin/win64")
    endif ()
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    if (_steam_bitness EQUAL 64)
        set(_steam_bin_dir "redistributable_bin/linux64")
    else ()
        set(_steam_bin_dir "redistributable_bin/linux32")
    endif ()
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(_steam_bin_dir "redistributable_bin/osx")
endif ()

# Find Headers
find_path(SteamworksSDK_INCLUDE_DIR
        NAMES steam_api.h
        PATHS ${Steamworks_ROOT}
        PATH_SUFFIXES "public/steam" "public"
        DOC "Steamworks SDK include directory")
mark_as_advanced(SteamworksSDK_INCLUDE_DIR)

# Find Library (.lib / .so / .dylib)
find_library(SteamworksSDK_LIBRARY
        NAMES ${_steam_lib_name}
        PATHS ${Steamworks_ROOT}
        PATH_SUFFIXES ${_steam_bin_dir}
        DOC "Steamworks SDK library")
mark_as_advanced(SteamworksSDK_LIBRARY)

# Find Binary (.dll / .so / .dylib)
if (WIN32)
    set(_steam_bin_name "${_steam_lib_name}.dll")
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(_steam_bin_name "lib${_steam_lib_name}.dylib")
else ()
    set(_steam_bin_name "lib${_steam_lib_name}.so")
endif ()

find_file(SteamworksSDK_BINARY
        NAMES ${_steam_bin_name}
        PATHS ${Steamworks_ROOT}
        PATH_SUFFIXES ${_steam_bin_dir}
        DOC "Steamworks SDK binary")
mark_as_advanced(SteamworksSDK_BINARY)

# Verify everything was found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SteamworksSDK
        REQUIRED_VARS SteamworksSDK_LIBRARY SteamworksSDK_INCLUDE_DIR SteamworksSDK_BINARY)

if (SteamworksSDK_FOUND)
    set(SteamworksSDK_INCLUDE_DIRS "${SteamworksSDK_INCLUDE_DIR}")
    set(SteamworksSDK_LIBRARIES "${SteamworksSDK_LIBRARY}")

    # Create the imported target
    if (NOT TARGET SteamworksSDK::API)
        add_library(SteamworksSDK::API SHARED IMPORTED)
        set_target_properties(SteamworksSDK::API
                PROPERTIES
                IMPORTED_LOCATION "${SteamworksSDK_BINARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${SteamworksSDK_INCLUDE_DIR}")

        # Windows requires an import lib (.lib) to link against a DLL
        if (WIN32)
            set_target_properties(SteamworksSDK::API
                    PROPERTIES
                    IMPORTED_IMPLIB "${SteamworksSDK_LIBRARY}")
        endif ()
    endif ()
endif ()

# Cleanup internal variables
unset(_steam_bitness)
unset(_steam_lib_name)
unset(_steam_bin_dir)
unset(_steam_bin_name)