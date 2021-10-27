cmake_minimum_required(VERSION 3.6)

if (FLAVOR STREQUAL "FULL")
    add_subdirectory(${CMAKE_SOURCE_DIR}/src/main/cpp)
endif ()
