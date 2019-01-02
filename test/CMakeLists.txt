# Tests for ssched_simulator
cmake_minimum_required(VERSION 3.0)

project(SSchedSimulatorTest)


# Catch2 output stream example from https://github.com/catchorg/Catch2/blob/master/examples/231-Cfg-OutputStreams.cpp

add_executable(cfg_outputstream_example
        "231-Cfg-OutputStreams.cc")
add_dependencies(cfg_outputstream_example
        catch2)

# Test cases for this project
set( TEST_SOURCES 
     test_main.cc
     test_traffic_generator.cc
        test_util.cc
        test_scheduler.cc)

add_executable(sschedsim_test
    ${TEST_SOURCES})
add_dependencies(sschedsim_test
    json
    catch2 )
target_link_libraries(sschedsim_test
        injection_model
        traffic_pattern
        scheduler
        switch
        ${saber_libraries})
add_test(NAME sschedsim_test COMMAND sschedsim_test)
