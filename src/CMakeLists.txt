cmake_minimum_required(VERSION 3.6)

project(godot_openxrmobile VERSION 1.0.0 LANGUAGES C CXX)
find_package(OpenXR)

MESSAGE("OpenXR found: ${OpenXR_FOUND}")

# set up common compile options
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -fexceptions -frtti")
set(CMAKE_CXX_STANDARD 14)

# TODO: do we need this?
# SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS true)

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

MESSAGE("Using ANDROID_STL: ${ANDROID_STL}")
MESSAGE("Using cmake toolchain ${CMAKE_TOOLCHAIN_FILE}")

###############################################################################
## PROJECT SETUP
## The variables below should be updated based on the setup instructions
## in the project README.

# Location to the Godot headers directory.
set(GODOT_CPP_DIR "${PROJECT_SOURCE_DIR}/../godot-cpp")
set(GODOT_HEADERS_DIR "${GODOT_CPP_DIR}/godot_headers")
###############################################################################

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

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -Wno-ignored-attributes")
endif ()

if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -fno-omit-frame-pointer -O0")
else ()
    set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -O3")
endif (CMAKE_BUILD_TYPE MATCHES Debug)


## godot-cpp library
set(GODOT-CPP "godot-cpp")

file(GLOB_RECURSE GODOT_CPP_SOURCES ${GODOT_CPP_DIR}/src/*.c**)
file(GLOB_RECURSE GODOT_CPP_HEADERS ${GODOT_CPP_DIR}/include/*.h**)

add_library(${GODOT-CPP}
        SHARED
        ${GODOT_CPP_SOURCES} ${GODOT_CPP_HEADERS})

target_include_directories(${GODOT-CPP}
        PRIVATE
        ${GODOT_CPP_DIR}/include
        ${GODOT_CPP_DIR}/include/core
        ${GODOT_CPP_DIR}/include/gen
        )

# Put godot headers as SYSTEM to exclude warnings from irrelevant headers
target_include_directories(${GODOT-CPP}
        SYSTEM PRIVATE
        ${GODOT_HEADERS_DIR}
        )

# Add the compile flags
set_property(TARGET ${GODOT-CPP} APPEND_STRING PROPERTY COMPILE_FLAGS ${GODOT_COMPILE_FLAGS})
set_property(TARGET ${GODOT-CPP} APPEND_STRING PROPERTY LINK_FLAGS ${GODOT_LINKER_FLAGS})

## Setup the plugin library
# Get Sources
file(GLOB SOURCES *.c *.cpp openxr/*.c openxr/*.cpp openxr/actions/*.c openxr/actions/*.cpp)
file(GLOB HEADERS *.h**)

MESSAGE("Plugin sources : ${SOURCES}")

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${GODOT_CPP_DIR}/src
        ${GODOT_CPP_DIR}/src/core
        ${GODOT_CPP_DIR}/src/gen
        ${GODOT_CPP_DIR}/include
        ${GODOT_CPP_DIR}/include/core
        ${GODOT_CPP_DIR}/include/gen
        )

# Put godot headers as SYSTEM to exclude warnings from irrelevant headers
target_include_directories(${PROJECT_NAME}
        SYSTEM PRIVATE
        "./"
        ${GODOT_HEADERS_DIR}
        ${OPENXR_HEADERS_DIR}
        )

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        #android
        #log
        EGL
        #GLESv3
        OpenXR::openxr_loader
        ${GODOT-CPP})

# TODO: do we need this?
# Add the compile flags
#set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS ${GODOT_COMPILE_FLAGS})
#set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS ${GODOT_LINKER_FLAGS})

get_target_property(OPENXR_LIB_PATH OpenXR::openxr_loader IMPORTED_LOCATION)
# Copy the OpenXR shared library to the output directory.
add_custom_command(OUTPUT OpenXR::openxr_loader
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OPENXR_LIB_PATH} $<TARGET_FILE_DIR:${PROJECT_NAME}>)
