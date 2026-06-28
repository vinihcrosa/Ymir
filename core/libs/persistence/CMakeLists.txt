add_library(ymir_persistence STATIC
    src/json/ScenarioReader.cpp
)
add_library(Ymir::Persistence ALIAS ymir_persistence)

target_include_directories(ymir_persistence
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/simulation/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ymir_persistence
    PUBLIC
        ymir_physics
        ymir_world
        ymir_vessel
        nlohmann_json::nlohmann_json
)

target_set_warnings(ymir_persistence)
