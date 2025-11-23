# ============================================================================
# SW-QPS HLS TCL Script - DIAGONAL TRAFFIC
# ============================================================================

# Open project
open_project -reset sw_qps_project_diagonal

# Set top function
set_top sw_qps_top

# Add source files
add_files src/sw_qps_top.cpp
add_files src/sliding_window.h
add_files src/input_port.h
add_files src/output_port.h
add_files src/qps_sampler.cpp
add_files src/utils.h
add_files src/sw_qps_types.h

# Add testbench files
add_files -tb tb/tb_sw_qps_diagonal.cpp
add_files -tb tb/tb_sw_qps_pure.cpp -cflags "-std=c++11 -DSW_QPS_PURE_DISABLE_MAIN"

# Open solution
open_solution "solution-diagonal"

# Target FPGA - Xilinx Zynq UltraScale+
set_part xczu9eg-ffvb1156-2-e

# Set clock period (5ns = 200 MHz)
create_clock -period 5 -name default

# ============================================================================
# Step 2: Synthesis
# ============================================================================
puts "========================================="
puts "Step 2: Running Synthesis - DIAGONAL..."
puts "========================================="

# Synthesis directives for optimization
config_compile -name_max_length 100
config_schedule
config_bind

# Run synthesis
csynth_design

# ============================================================================
# Step 3: Co-Simulation
# ============================================================================
puts "========================================="
puts "Step 3: Running Co-Simulation - DIAGONAL..."
puts "========================================="

# Faster option: Run with reduced test set
cosim_design -O -trace_level all

puts "========================================="
puts "DIAGONAL Traffic Flow Complete!"
puts "========================================="
