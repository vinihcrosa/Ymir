add_executable(ymir_tests
    SmokeTest.cpp
    core/TestMath.cpp
    core/TestForces.cpp
    core/TestBody.cpp
    core/TestForceModel.cpp
    core/TestIntegrator.cpp
    core/TestSimulationIntegration.cpp
)

target_link_libraries(ymir_tests
    PRIVATE
        ymir_core
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(ymir_tests)

# Naval tests
add_executable(ymir_naval_tests
    naval/TestNavalInfra.cpp
    naval/TestInertialForces.cpp
    naval/TestRestoringForces.cpp
    naval/TestDampingForces.cpp
    naval/TestSquatForces.cpp
    naval/TestWindForces.cpp
    naval/TestCurrentForces.cpp
    naval/TestWaveSpectrum.cpp
    naval/TestWaveForces.cpp
    naval/TestThrustForces.cpp
    naval/TestRudderForces.cpp
    naval/TestTugForces.cpp
    naval/TestNavalIntegration.cpp
)

target_link_libraries(ymir_naval_tests
    PRIVATE
        ymir_naval
        Catch2::Catch2WithMain
)

catch_discover_tests(ymir_naval_tests)

# Adapters tests
add_executable(ymir_adapters_tests
    adapters/TestScenarioReader.cpp
)

target_link_libraries(ymir_adapters_tests
    PRIVATE
        ymir_adapters
        Catch2::Catch2WithMain
)

target_compile_definitions(ymir_adapters_tests
    PRIVATE YMIR_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples"
)

catch_discover_tests(ymir_adapters_tests)
