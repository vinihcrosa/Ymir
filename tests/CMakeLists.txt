include(CTest)
include(Catch)

add_executable(ymir_smoke_tests
    SmokeTest.cpp
)

target_link_libraries(ymir_smoke_tests
    PRIVATE
        Ymir::Physics
        Catch2::Catch2WithMain
)

catch_discover_tests(ymir_smoke_tests)

add_executable(ymir_common_tests
    common/TestMath.cpp
)

target_link_libraries(ymir_common_tests
    PRIVATE
        Ymir::Common
        Catch2::Catch2WithMain
)

catch_discover_tests(ymir_common_tests)

add_executable(ymir_physics_tests
    physics/TestBody.cpp
    physics/TestForceModel.cpp
    physics/TestForces.cpp
    physics/forces/TestCurrentForces.cpp
    physics/forces/TestDampingForces.cpp
    physics/forces/TestInertialForces.cpp
    physics/forces/TestRestoringForces.cpp
    physics/forces/TestRudderForces.cpp
    physics/forces/TestSquatForces.cpp
    physics/forces/TestThrustForces.cpp
    physics/forces/TestTugForces.cpp
    physics/forces/TestWindForces.cpp
)

target_link_libraries(ymir_physics_tests
    PRIVATE
        Ymir::Physics
        Catch2::Catch2WithMain
)

catch_discover_tests(ymir_physics_tests)

add_executable(ymir_integrator_tests
    physics/TestIntegrator.cpp
    physics/TestIntegratorRK45.cpp
)

target_link_libraries(ymir_integrator_tests
    PRIVATE
        Ymir::Simulation
        Catch2::Catch2WithMain
)

catch_discover_tests(ymir_integrator_tests)

add_executable(ymir_simulation_tests
    simulation/TestNavalInfra.cpp
    simulation/TestNavalIntegration.cpp
    simulation/TestSimulationIntegration.cpp
    simulation/TestVesselIntegration.cpp
    simulation/TestCouplingForceModel.cpp
    simulation/TestNavalDomain.cpp
    simulation/TestNavalDomainCoupling.cpp
)

target_link_libraries(ymir_simulation_tests
    PRIVATE
        Ymir::Simulation
        Catch2::Catch2WithMain
)

catch_discover_tests(ymir_simulation_tests)

add_executable(ymir_world_tests
    world/TestEnvironment.cpp
    world/TestIDomain.cpp
    world/TestCouplingRegistry.cpp
    world/TestWorld.cpp
    world/TestWaveForces.cpp
    world/TestWaveSpectrum.cpp
)

target_link_libraries(ymir_world_tests
    PRIVATE
        Ymir::World
        Catch2::Catch2WithMain
)

catch_discover_tests(ymir_world_tests)

add_executable(ymir_persistence_tests
    persistence/TestScenarioReader.cpp
)

target_link_libraries(ymir_persistence_tests
    PRIVATE
        Ymir::Persistence
        Ymir::Simulation
        Catch2::Catch2WithMain
)

target_compile_definitions(ymir_persistence_tests
    PRIVATE YMIR_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples"
)

catch_discover_tests(ymir_persistence_tests)

if(YMIR_ENABLE_COVERAGE)
    find_program(LLVM_PROFDATA_EXECUTABLE llvm-profdata)
    find_program(LLVM_COV_EXECUTABLE llvm-cov)

    if(NOT LLVM_PROFDATA_EXECUTABLE OR NOT LLVM_COV_EXECUTABLE)
        find_program(XCRUN_EXECUTABLE xcrun REQUIRED)
        execute_process(
            COMMAND ${XCRUN_EXECUTABLE} --find llvm-profdata
            OUTPUT_VARIABLE LLVM_PROFDATA_EXECUTABLE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
        execute_process(
            COMMAND ${XCRUN_EXECUTABLE} --find llvm-cov
            OUTPUT_VARIABLE LLVM_COV_EXECUTABLE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()

    set(YMIR_COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(YMIR_COVERAGE_PROFDATA "${YMIR_COVERAGE_DIR}/ymir.profdata")
    set(YMIR_COVERAGE_TARGETS
        ymir_smoke_tests
        ymir_common_tests
        ymir_physics_tests
        ymir_integrator_tests
        ymir_simulation_tests
        ymir_world_tests
        ymir_persistence_tests
    )

    add_custom_target(ymir_coverage
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${YMIR_COVERAGE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${YMIR_COVERAGE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E env
            LLVM_PROFILE_FILE=${YMIR_COVERAGE_DIR}/%p.profraw
            ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_BINARY_DIR}" --output-on-failure
        COMMAND /bin/sh -c
            "\"${LLVM_PROFDATA_EXECUTABLE}\" merge -sparse \"${YMIR_COVERAGE_DIR}\"/*.profraw -o \"${YMIR_COVERAGE_PROFDATA}\""
        COMMAND "${LLVM_COV_EXECUTABLE}" report
            "$<TARGET_FILE:ymir_smoke_tests>"
            -object "$<TARGET_FILE:ymir_common_tests>"
            -object "$<TARGET_FILE:ymir_physics_tests>"
            -object "$<TARGET_FILE:ymir_integrator_tests>"
            -object "$<TARGET_FILE:ymir_simulation_tests>"
            -object "$<TARGET_FILE:ymir_world_tests>"
            -object "$<TARGET_FILE:ymir_persistence_tests>"
            "-instr-profile=${YMIR_COVERAGE_PROFDATA}"
            -ignore-filename-regex
            "(^|.*/)(build[^/]*|_deps|tests)/.*"
        DEPENDS
            ${YMIR_COVERAGE_TARGETS}
        VERBATIM
        USES_TERMINAL
        COMMENT "Running Ymir tests with LLVM source coverage"
    )
endif()
