add_library(ymir_world STATIC
    src/Environment.cpp
    src/CouplingRegistry.cpp
    src/EnvironmentTimeline.cpp
    src/World.cpp
    src/wave/WaveComponent.cpp
    src/wave/WaveForces.cpp
    src/wave/WaveSpectrum.cpp
)
add_library(Ymir::World ALIAS ymir_world)

target_include_directories(ymir_world
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/vessel/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ymir_world
    PUBLIC
        ymir_physics
        ymir_common
        nlohmann_json::nlohmann_json  # JSON parsing — nlohmann/json v3.11.3 (header-only, MIT)
)

target_set_warnings(ymir_world)
