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
option(YMIR_ENABLE_COVERAGE "Enable LLVM source coverage instrumentation" OFF)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_BINARY_DIR}/include/ymir/version.h
)

include(cmake/Dependencies.cmake)
include(cmake/CompilerWarnings.cmake)

if(YMIR_ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "YMIR_ENABLE_COVERAGE requires Clang or AppleClang")
    endif()

    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate -fcoverage-mapping)
endif()

if(YMIR_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(libs/common)
add_subdirectory(libs/physics)
add_subdirectory(libs/simulation)
add_subdirectory(libs/world)
add_subdirectory(libs/vessel)
add_subdirectory(libs/persistence)

if(YMIR_BUILD_APPS)
    add_subdirectory(apps/server)
    add_subdirectory(apps/fast-time)
endif()

if(YMIR_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(YMIR_BUILD_DOCS)
    find_package(Doxygen OPTIONAL_COMPONENTS dot)
    if(DOXYGEN_FOUND)
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    else()
        message(STATUS "Doxygen not found — 'docs' target unavailable")
    endif()
endif()
