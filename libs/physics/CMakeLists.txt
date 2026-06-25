add_library(ymir_physics STATIC
    src/physics.cpp
    src/naval_physics.cpp
    src/RigidBody6DOF.cpp
    src/NavalForceModel.cpp
    src/integrator/CvodeIntegrator.cpp
    src/forces/CurrentForces.cpp
    src/forces/DampingForces.cpp
    src/forces/InertialForces.cpp
    src/forces/RestoringForces.cpp
    src/forces/RudderForces.cpp
    src/forces/SquatForces.cpp
    src/forces/ThrustForces.cpp
    src/forces/TugForces.cpp
    src/forces/WindForces.cpp
)
add_library(Ymir::Physics ALIAS ymir_physics)

target_include_directories(ymir_physics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/vessel/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ymir_physics
    PUBLIC
        ymir_common
        SUNDIALS::cvode
)

target_set_warnings(ymir_physics)
