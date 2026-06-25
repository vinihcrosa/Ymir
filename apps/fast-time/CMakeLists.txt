add_library(ymir_fast_time_app INTERFACE)

target_link_libraries(ymir_fast_time_app
    INTERFACE
        Ymir::Simulation
        Ymir::World
        Ymir::Vessel
)
