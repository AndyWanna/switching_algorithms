# Auto-generated TCL file for HLS
open_project -reset phase1_test

set_top test_phase1_top

add_files src/top.cpp
add_files src/qps_sampler.cpp
add_files src/utils.h
add_files src/sw_qps_types.h

add_files -tb tb/tb_phase1.cpp

open_solution "solution1"

set_part xczu9eg-ffvb1156-2-e

create_clock -period 10 -name default

# Run C simulation
puts "========================================="
puts "Running C Simulation..."
puts "========================================="
csim_design -clean

# Run synthesis
puts "========================================="
puts "Running Synthesis..."
puts "========================================="
csynth_design

# Run co-simulation
puts "========================================="
puts "Running Co-Simulation..."
puts "========================================="
cosim_design

exit