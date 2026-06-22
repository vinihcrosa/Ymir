add_library(ymir_core STATIC
    src/core.cpp
    src/Body.cpp
    src/Simulation.cpp
    src/integrator/CvodeIntegrator.cpp
)

target_include_directories(ymir_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ymir_core
    PUBLIC SUNDIALS::cvode
)

target_set_warnings(ymir_core)
