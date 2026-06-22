add_library(ymir_core STATIC
    src/core.cpp
)

target_include_directories(ymir_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_set_warnings(ymir_core)
