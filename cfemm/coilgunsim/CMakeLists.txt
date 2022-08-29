add_library(coilgunsim STATIC 
    FemmAPI.cpp
    FemmAPI.h
    FemmExtensions.h
    CoilGunSim.h
    CoilGunSim.cpp
    CoilGunSim.Simulation.cpp
    )
target_include_directories(coilgunsim PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/femmcli $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/libfemm $<INSTALL_INTERFACE:include>)
target_link_libraries(coilgunsim
    PUBLIC femm fmesher
    esolver epproc
    fsolver fpproc
    hsolver hpproc
    )

add_executable(coilgunsim-bin
    main.cpp
    )

set_target_properties(coilgunsim-bin
    PROPERTIES OUTPUT_NAME coilgunsim)

target_link_libraries(coilgunsim-bin coilgunsim)
install(
    TARGETS coilgunsim-bin
    RUNTIME DESTINATION bin
    COMPONENT "cli")
# vi:expandtab:tabstop=4 shiftwidth=4:
