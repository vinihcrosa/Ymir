add_library(ymir_common INTERFACE)
add_library(Ymir::Common ALIAS ymir_common)

target_include_directories(ymir_common
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
