cmake_minimum_required(VERSION 3.16)

project(Ymir
    VERSION 0.1.0
    DESCRIPTION "General-purpose physics engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(YMIR_BUILD_TESTS  "Build test suite"      ON)
option(YMIR_BUILD_DOCS   "Generate Doxygen docs" OFF)
option(YMIR_BUILD_APPS   "Build applications"    OFF)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_BINARY_DIR}/include/ymir/version.h
)

include(cmake/Dependencies.cmake)
include(cmake/CompilerWarnings.cmake)

add_subdirectory(core)
add_subdirectory(naval)
add_subdirectory(adapters)

if(YMIR_BUILD_APPS)
    add_subdirectory(applications)
endif()

if(YMIR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
