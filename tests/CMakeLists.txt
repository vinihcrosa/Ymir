add_executable(ymir_tests
    SmokeTest.cpp
)

target_link_libraries(ymir_tests
    PRIVATE
        ymir_core
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(ymir_tests)
