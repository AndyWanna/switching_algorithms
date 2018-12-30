# Tests for ssched_simulator
cmake_minimum_required(VERSION 3.0)

project(SSchedSimulatorExample)



# Examples for this project
set(saber_examples "")
add_executable(traffic_generator_example
        "traffic_generator_example.cc")
add_dependencies(traffic_generator_example
        json
        injection_model
        traffic_pattern)

target_link_libraries(traffic_generator_example
        injection_model
        traffic_pattern)
list(APPEND saber_examples
        traffic_generator_example )



add_custom_target(
        example
        COMMAND $<TARGET_FILE:traffic_generator_example>
        DEPENDS ${saber_examples}
)