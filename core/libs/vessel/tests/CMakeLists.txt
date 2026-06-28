include(CTest)
include(Catch)

add_executable(ymir_vessel_tests
    TestThruster.cpp
    TestRudder.cpp
    TestVesselState.cpp
    TestPrescribedController.cpp
    TestManeuverController.cpp
    TestBerthManeuverSystem.cpp
    TestDynamicVessel.cpp
)

target_link_libraries(ymir_vessel_tests
    PRIVATE
        Ymir::Vessel
        Ymir::Physics
        Ymir::Simulation
        Catch2::Catch2WithMain
)

catch_discover_tests(ymir_vessel_tests)
