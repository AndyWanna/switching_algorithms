# ============================================================================
# SW-QPS HLS TCL Script
# ============================================================================
# 
# Runs complete SW-QPS synthesis flow:
#   1. C simulation
#   2. Synthesis  
#   3. Co-simulation
#   4. Export IP (optional)

# Open project
open_project -reset sw_qps_project_aggressive

# Set top function
set_top sw_qps_top

# Add source files
add_files src_aggressive/sw_qps_top.cpp
add_files src_aggressive/sliding_window.h
add_files src_aggressive/input_port.h
add_files src_aggressive/output_port.h
add_files src_aggressive/qps_sampler.cpp
add_files src_aggressive/utils.h
add_files src_aggressive/sw_qps_types.h

# Add testbench files
add_files -tb tb/tb_sw_qps_hls.cpp
add_files -tb tb/tb_sw_qps_pure.cpp -cflags "-std=c++11 -DSW_QPS_PURE_DISABLE_MAIN"

# Open solution
open_solution "solution-aggressive"

# Target FPGA - Xilinx Zynq UltraScale+ (can adjust based on board)
set_part xczu9eg-ffvb1156-2-e

# Alternative parts:
# set_part xczu7ev-ffvc1156-2-e  # Zynq UltraScale+ EV
# set_part xc7z020-clg484-1      # Zynq-7000 (Zedboard/PYNQ)
# set_part xcvu9p-flga2104-2L-e  # Virtex UltraScale+

# Set clock period (5ns = 200 MHz)
create_clock -period 5 -name default

# Alternative clock speeds:
# create_clock -period 10 -name default  # 100 MHz
# create_clock -period 4 -name default   # 250 MHz
# create_clock -period 3.33 -name default # 300 MHz

# ============================================================================
# Step 1: C Simulation
# ============================================================================
puts "========================================="
puts "Step 1: Running C Simulation..."
puts "========================================="
csim_design -clean

# ============================================================================
# Step 2: Synthesis
# ============================================================================
puts "========================================="
puts "Step 2: Running Synthesis..."
puts "========================================="

# Synthesis directives for optimization
config_compile -name_max_length 100
config_schedule
config_bind

# Run synthesis
csynth_design

# ============================================================================
# Step 3: Co-Simulation (Optional - can be slow)
# ============================================================================
# puts "========================================="
# puts "Step 3: Running Co-Simulation..."
# puts "========================================="

# Configure co-simulation
# Note: Co-sim can be very slow for large designs
# Uncomment to enable:
# cosim_design -trace_level all -rtl vhdl -tool xsim

# Faster option: Run with reduced test set
# cosim_design -O -trace_level all

# # ============================================================================
# # Step 4: Export IP (Optional)
# # ============================================================================
# puts "========================================="
# puts "Step 4: Exporting IP..."
# puts "========================================="

# # Export as Vivado IP
# # export_design -format ip_catalog -description "SW-QPS Switching Algorithm" -display_name "SW_QPS"

# # Export for System Generator
# # export_design -format sysgen

# # ============================================================================
# # Step 5: Report Generation
# # ============================================================================
# puts "========================================="
# puts "Step 5: Generating Reports..."
# puts "========================================="

# # Additional reports can be generated here
# # report_latency
# # report_throughput
# # report_area
# # report_timing

# puts "========================================="
# puts "SW-QPS HLS Flow Complete!"
# puts "========================================="
# puts "Check the following reports:"
# puts "  - solution1/syn/report/sw_qps_top_csynth.rpt (Synthesis Report)"
# puts "  - solution1/sim/report/sw_qps_top_csim.log (C Simulation Log)"
# puts "  - solution1/sim/report/vhdl/sw_qps_top.log (Co-Simulation Log)"
# puts "========================================="

# # Exit
exit
