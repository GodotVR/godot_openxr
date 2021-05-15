# Copyright (c) 2017 The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# This is a generated file - do not edit!

unset(_UWP_SUFFIX)
if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    set(_UWP_SUFFIX _uwp)
endif()
if(CMAKE_GENERATOR_PLATFORM_UPPER MATCHES "ARM.*")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_PLATFORM ARM64)
    else()
        set(_PLATFORM ARM)
    endif()
else()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_PLATFORM x64)
    else()
        set(_PLATFORM Win32)
    endif()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/${_PLATFORM}${_UWP_SUFFIX}/lib/cmake/openxr/OpenXRConfig.cmake")
