add_library(ymir_adapters STATIC
    src/adapters.cpp
)

target_include_directories(ymir_adapters
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ymir_adapters
    PUBLIC
        ymir_core
        ymir_naval
        nlohmann_json::nlohmann_json
)

target_set_warnings(ymir_adapters)
