# ============================================================================
# SW-QPS HLS TCL Script - DIAGONAL TRAFFIC (AGGRESSIVE)
# ============================================================================

# Open project
open_project -reset sw_qps_project_diagonal_aggressive

# Set top function
set_top sw_qps_top

# Add source files from aggressive directory
add_files src_aggressive/sw_qps_top.cpp
add_files src_aggressive/sliding_window.h
add_files src_aggressive/input_port.h
add_files src_aggressive/output_port.h
add_files src_aggressive/qps_sampler.cpp
add_files src_aggressive/utils.h
add_files src_aggressive/sw_qps_types.h

# Add testbench files
add_files -tb tb/tb_sw_qps_diagonal.cpp
add_files -tb tb/tb_sw_qps_pure.cpp -cflags "-std=c++11 -DSW_QPS_PURE_DISABLE_MAIN"

# Open solution
open_solution "solution-diagonal-aggressive"

# Target FPGA - Xilinx Zynq UltraScale+
set_part xczu9eg-ffvb1156-2-e

# Set clock period (5ns = 200 MHz)
create_clock -period 5 -name default

# ============================================================================
# Step 2: Synthesis
# ============================================================================
puts "========================================="
puts "Step 2: Running Synthesis - DIAGONAL AGGRESSIVE..."
puts "========================================="

# Aggressive synthesis directives
config_compile -name_max_length 100
config_schedule -enable_dsp_full_reg=1
config_bind

# Run synthesis
csynth_design

# ============================================================================
# Step 3: Co-Simulation
# ============================================================================
puts "========================================="
puts "Step 3: Running Co-Simulation - DIAGONAL AGGRESSIVE..."
puts "========================================="

cosim_design -O -trace_level all

puts "========================================="
puts "DIAGONAL AGGRESSIVE Traffic Flow Complete!"
puts "========================================="
