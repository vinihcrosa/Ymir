add_library(ymir_naval STATIC
    src/naval.cpp
)

target_include_directories(ymir_naval
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ymir_naval
    PUBLIC ymir_core
)

target_set_warnings(ymir_naval)
