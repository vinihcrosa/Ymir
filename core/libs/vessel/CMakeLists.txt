add_library(ymir_vessel STATIC
    src/Thruster.cpp
    src/Rudder.cpp
    src/DynamicVessel.cpp
    src/controllers/ManeuverController.cpp
    src/controllers/PrescribedController.cpp
    src/controllers/BerthManeuverSystem.cpp
    src/controllers/TugParametricForces.cpp
)
add_library(Ymir::Vessel ALIAS ymir_vessel)

target_include_directories(ymir_vessel
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/world/include>
)

target_link_libraries(ymir_vessel
    PUBLIC
        ymir_physics
        ymir_common
)

if(YMIR_BUILD_TESTS)
    add_subdirectory(tests)
endif()
