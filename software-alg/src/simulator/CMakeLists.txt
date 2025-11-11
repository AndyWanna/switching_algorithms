cmake_minimum_required(VERSION 3.0)

set(simulator_hdr
        "simulator.h"
        "simulator_factory.h")
set(simulator_src
        "simulator.cc"
        "simulator_factory.cc")
add_library(simulator
        ${simulator_hdr}
        ${simulator_src})
target_link_libraries(simulator
        injection_model
        traffic_pattern
        scheduler
        switch
        ${saber_libraries})

