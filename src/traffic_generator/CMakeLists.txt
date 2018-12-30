cmake_minimum_required(VERSION 3.2)


# library for injection model
set(injection_model_hdr 
    "injection_model.h" 
    "bernoulli_injection_model.h" 
    "onoff_injection_model.h"
    "injection_model_factory.h"
    )
set(injection_model_src 
    "injection_model.cc" 
    "bernoulli_injection_model.cc" 
    "onoff_injection_model.cc"
    "injection_model_factory.cc"
    )

add_library(injection_model 
    "${injection_model_hdr}"  
    "${injection_model_src}")
add_dependencies(injection_model
        json )
list(APPEND saber_libraries injection_model)

# library for traffic pattern
set(traffic_pattern_hdr
        "traffic_pattern.h"
        "uniform_traffic_pattern.h"
        "log_diagonal_traffic_pattern.h"
        "quasi_diagonal_traffic_pattern.h"
        "diagonal_traffic_pattern.h"
        "traffic_pattern_factory.h" )
set(traffic_pattern_src
        "traffic_pattern.cc"
        "uniform_traffic_pattern.cc"
        "log_diagonal_traffic_pattern.cc"
        "quasi_diagonal_traffic_pattern.cc"
        "diagonal_traffic_pattern.cc"
        "traffic_pattern_factory.cc" )

add_library(traffic_pattern
        "${traffic_pattern_hdr}"
        "${traffic_pattern_src}")
add_dependencies(traffic_pattern
        json )
list(APPEND saber_libraries traffic_pattern)





