add_library(ymir_naval STATIC
    src/NavalSimulation.cpp
    src/NavalForceModel.cpp
    src/forces/InertialForces.cpp
    src/forces/RestoringForces.cpp
    src/forces/DampingForces.cpp
    src/forces/SquatForces.cpp
    src/forces/WindForces.cpp
    src/forces/CurrentForces.cpp
    src/forces/ThrustForces.cpp
    src/forces/RudderForces.cpp
    src/forces/TugForces.cpp
    src/wave/WaveSpectrum.cpp
    src/wave/WaveComponent.cpp
    src/wave/WaveForces.cpp
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
