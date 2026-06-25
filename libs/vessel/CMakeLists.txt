add_library(ymir_vessel INTERFACE)
add_library(Ymir::Vessel ALIAS ymir_vessel)

target_include_directories(ymir_vessel
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ymir_vessel
    INTERFACE
        ymir_physics
        ymir_common
)
