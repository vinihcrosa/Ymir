add_library(ymir_server_app INTERFACE)

target_link_libraries(ymir_server_app
    INTERFACE
        Ymir::Simulation
        Ymir::Persistence
)
