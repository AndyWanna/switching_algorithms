cmake_minimum_required(VERSION 3.0)


# library
set(switch_hdr
        "switch.h"
        "iq_switch.h"
        "iq_switch_factory.h")
set(switch_src
        "switch.cc"
        "iq_switch.cc"
        "iq_switch_factory.cc")
add_library(switch
        ${switch_hdr}
        ${switch_src})
add_dependencies(switch
        json)
target_link_libraries(switch
        scheduler
        ${saber_libraries})

