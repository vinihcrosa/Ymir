add_executable(ymir_tests
    SmokeTest.cpp
    core/TestMath.cpp
    core/TestForces.cpp
    core/TestBody.cpp
    core/TestForceModel.cpp
    core/TestIntegrator.cpp
)

target_link_libraries(ymir_tests
    PRIVATE
        ymir_core
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(ymir_tests)
