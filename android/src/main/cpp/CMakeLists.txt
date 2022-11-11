cmake_minimum_required(VERSION 3.6)

# Default android abi is arm64-v8a
if (NOT ANDROID_ABI)
    set(ANDROID_ABI "arm64-v8a")
endif (NOT ANDROID_ABI)

# Default android platform is android-24
if (NOT ANDROID_PLATFORM)
    set(ANDROID_PLATFORM "android-24")
endif (NOT ANDROID_PLATFORM)

# Default build type is Debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-D_DEBUG)
    set(OPENXR_MOBILE_LIB_BUILD_TYPE Debug)
else ()
    add_definitions(-DNDEBUG)
    set(OPENXR_MOBILE_LIB_BUILD_TYPE Release)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if (NOT (ANDROID_STL STREQUAL "c++_shared"))
    set(ANDROID_STL "c++_shared")
endif (NOT (ANDROID_STL STREQUAL "c++_shared"))

# Check if ANDROID_NDK is set.
if (NOT ANDROID_NDK)
    # Set to ANDROID_NDK_HOME environment variable if it's set.
    if (DEFINED ENV{ANDROID_NDK_HOME})
        set(ANDROID_NDK $ENV{ANDROID_NDK_HOME})
    else (DEFINED ENV{ANDROID_NDK_HOME})
        message(WARNING "ANDROID_NDK_HOME is not set")
    endif (DEFINED ENV{ANDROID_NDK_HOME})
endif (NOT ANDROID_NDK)

# Check if CMAKE_TOOLCHAIN_FILE is set.
if (NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${ANDROID_NDK}/build/cmake/android.toolchain.cmake")
endif (NOT CMAKE_TOOLCHAIN_FILE)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(godot_openxr VERSION 2.0.0)

###############################################################################
## PROJECT SETUP
## The variables below should be updated based on the setup instructions
## in the project README.

# Sets the path to the OpenXR mobile library containing directory. Update this variable if the api is in another location.
set(OPENXR_MOBILE_ROOT_DIR "${CMAKE_SOURCE_DIR}/../thirdparty/oculus_mobile_sdk/OpenXR")

# Location to the Godot-cpp directory.
set(GODOT_CPP_DIR "${CMAKE_SOURCE_DIR}/../thirdparty/godot-cpp")
set(GODOT_HEADERS_DIR "${GODOT_CPP_DIR}/godot-headers")
###############################################################################

add_definitions(-DANDROID)

set(OPENXR_HEADERS_DIR "${CMAKE_SOURCE_DIR}/../thirdparty/openxr_loader/include")
set(OPENXR_MOBILE_HEADERS_DIR "${OPENXR_MOBILE_ROOT_DIR}/Include" CACHE STRING "")

set(GODOT_COMPILE_FLAGS)
set(GODOT_LINKER_FLAGS)

set(GODOT_LINKER_FLAGS "-Wl")

set(GODOT_COMPILE_FLAGS "-fPIC -g -Wwrite-strings")
set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -Wchar-subscripts -Wcomment -Wdisabled-optimization")
set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -Wformat -Wformat=2 -Wformat-security -Wformat-y2k")
set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -Wimport -Winit-self -Winline -Winvalid-pch")
set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -Wlong-long -Wmissing-braces -Wmissing-format-attribute")
set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -Wmissing-include-dirs -Wmissing-noreturn -Wpacked -Wpointer-arith")
set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -Wredundant-decls -Wreturn-type -Wsequence-point")
set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -Wswitch -Wswitch-enum -Wtrigraphs")
set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -Wuninitialized -Wunknown-pragmas -Wunreachable-code -Wunused-label")
set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -Wunused-value -Wvariadic-macros -Wvolatile-register-var -Wno-error=attributes")
set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -fno-omit-frame-pointer -O0")

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -Wno-ignored-attributes")
endif ()

if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -fno-omit-frame-pointer -O0")
else ()
    set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -O3")
endif (CMAKE_BUILD_TYPE MATCHES Debug)

## OpenXR Mobile loader library
set(OPENXR_MOBILE_LIB_PATH "${OPENXR_MOBILE_ROOT_DIR}/Libs/Android/${ANDROID_ABI}/${OPENXR_MOBILE_LIB_BUILD_TYPE}/libopenxr_loader.so")
add_library(openxr_mobile_loader SHARED IMPORTED GLOBAL)
set_target_properties(openxr_mobile_loader PROPERTIES IMPORTED_LOCATION ${OPENXR_MOBILE_LIB_PATH})

## godot-cpp library
set(GODOT-CPP "godot-cpp")

file(GLOB_RECURSE GODOT_CPP_SOURCES ${GODOT_CPP_DIR}/src/*.c**)
file(GLOB_RECURSE GODOT_CPP_HEADERS ${GODOT_CPP_DIR}/include/*.h**)

add_library(${GODOT-CPP}
        STATIC
        ${GODOT_CPP_SOURCES} ${GODOT_CPP_HEADERS})

target_include_directories(${GODOT-CPP}
        PUBLIC
        ${GODOT_CPP_DIR}/include
        ${GODOT_CPP_DIR}/include/core
        ${GODOT_CPP_DIR}/include/gen
        )

# Put godot headers as SYSTEM PUBLIC to exclude warnings from irrelevant headers
target_include_directories(${GODOT-CPP}
        SYSTEM PUBLIC
        ${GODOT_HEADERS_DIR}
        )

# Add the compile flags
set_property(TARGET ${GODOT-CPP} APPEND_STRING PROPERTY COMPILE_FLAGS ${GODOT_COMPILE_FLAGS})
set_property(TARGET ${GODOT-CPP} APPEND_STRING PROPERTY LINK_FLAGS ${GODOT_LINKER_FLAGS})

## Setup the plugin library
# Get Sources
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/../src/*.c**)
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/../src/*.h**)
file(GLOB_RECURSE ANDROID_SOURCES ${CMAKE_SOURCE_DIR}/src/main/cpp/*.c**)
file(GLOB_RECURSE ANDROID_HEADERS ${CMAKE_SOURCE_DIR}/src/main/cpp/*.h**)

add_library(${PROJECT_NAME}
        SHARED
        ${SOURCES} ${HEADERS} ${ANDROID_SOURCES} ${ANDROID_HEADERS})

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/../src
        ${CMAKE_SOURCE_DIR}/src/main/cpp
        ${GODOT_CPP_DIR}/src
        ${GODOT_CPP_DIR}/src/core
        ${GODOT_CPP_DIR}/src/gen
        ${GODOT_CPP_DIR}/include
        ${GODOT_CPP_DIR}/include/core
        ${GODOT_CPP_DIR}/include/gen
        )

# Put godot headers as SYSTEM PUBLIC to exclude warnings from irrelevant headers
target_include_directories(${PROJECT_NAME}
        SYSTEM PUBLIC
        ${GODOT_HEADERS_DIR}
        ${OPENXR_HEADERS_DIR}
        ${OPENXR_MOBILE_HEADERS_DIR}
        )

target_link_libraries(${PROJECT_NAME}
        android
        log
        EGL
        GLESv3
        openxr_mobile_loader
        ${GODOT-CPP})

# Add the compile flags
set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS ${GODOT_COMPILE_FLAGS})
set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS ${GODOT_LINKER_FLAGS})

set(BITS 32)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BITS 64)
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)
