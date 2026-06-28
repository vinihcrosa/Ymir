add_library(ymir_simulation STATIC
    src/Simulation.cpp
    src/NavalSimulation.cpp
    src/NavalDomain.cpp
)
add_library(Ymir::Simulation ALIAS ymir_simulation)

target_include_directories(ymir_simulation
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/world/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/vessel/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ymir_simulation
    PUBLIC
        ymir_physics
        ymir_vessel
        ymir_world
        ymir_common
)

target_set_warnings(ymir_simulation)
