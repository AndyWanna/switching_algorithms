#!/bin/bash

# ============================================================================
# SW-QPS Build and Test Script
# ============================================================================

set -e  # Exit on error

echo "========================================"
echo "SW-QPS Build and Test Script"
echo "========================================"

# Configuration
HLS_TOOL="vitis_hls"  # or vivado_hls for older versions
PROJECT_DIR="hardware-hls"
RESULTS_DIR="results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create results directory
mkdir -p ${RESULTS_DIR}

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to check if HLS tool is available
check_hls_tool() {
    if command -v ${HLS_TOOL} &> /dev/null; then
        print_status "Found HLS tool: ${HLS_TOOL}"
        return 0
    else
        print_warning "HLS tool ${HLS_TOOL} not found in PATH"
        return 1
    fi
}

# Function to compile pure C++ testbench
compile_pure_cpp() {
    print_status "Compiling pure C++ testbench..."
    
    cd ${PROJECT_DIR}
    
    g++ -std=c++11 -O2 -Wall \
        -I src/ \
        tb/tb_sw_qps_pure.cpp \
        -o ${RESULTS_DIR}/sw_qps_pure_test
    
    if [ $? -eq 0 ]; then
        print_status "Pure C++ compilation successful"
    else
        print_error "Pure C++ compilation failed"
        return 1
    fi
    
    cd ..
}

# Function to run pure C++ tests
run_pure_cpp() {
    print_status "Running pure C++ tests..."
    
    ./${RESULTS_DIR}/sw_qps_pure_test | tee ${RESULTS_DIR}/pure_cpp_results.log
    
    if [ $? -eq 0 ]; then
        print_status "Pure C++ tests passed"
    else
        print_error "Pure C++ tests failed"
        return 1
    fi
}

# Function to run HLS synthesis
run_hls_synthesis() {
    if check_hls_tool; then
        print_status "Running HLS synthesis..."
        
        cd ${PROJECT_DIR}
        ${HLS_TOOL} -f run_sw_qps.tcl | tee ../${RESULTS_DIR}/hls_synthesis.log
        cd ..
        
        print_status "HLS synthesis complete. Check ${RESULTS_DIR}/hls_synthesis.log"
    else
        print_warning "Skipping HLS synthesis (tool not found)"
    fi
}

# Function to run performance analysis
run_performance_analysis() {
    print_status "Running performance analysis..."
    
    # Compile network simulator
    g++ -std=c++17 -O3 -pthread \
        sw_qps_simulator.cpp \
        -o ${RESULTS_DIR}/sw_qps_network_sim
    
    if [ $? -eq 0 ]; then
        print_status "Network simulator compiled"
        
        # Run simulation
        ./${RESULTS_DIR}/sw_qps_network_sim | tee ${RESULTS_DIR}/network_sim_results.log
        
        print_status "Performance analysis complete"
    else
        print_warning "Network simulator compilation failed"
    fi
}

# Function to generate reports
generate_reports() {
    print_status "Generating reports..."
    
    # Check if Python is available for plotting
    if command -v python3 &> /dev/null; then
        if [ -f "sw_qps_results.csv" ]; then
            python3 plot_results.py sw_qps_results.csv
            print_status "Performance plots generated"
        fi
    else
        print_warning "Python not found, skipping plot generation"
    fi
    
    # Generate summary report
    cat > ${RESULTS_DIR}/summary_report.txt << EOF
SW-QPS Implementation Summary Report
=====================================
Date: $(date)
Configuration:
  - N = 64 ports
  - T = 16 time slots
  - Knockout threshold = 3

Test Results:
-------------
EOF
    
    # Extract key metrics from logs
    if [ -f "${RESULTS_DIR}/pure_cpp_results.log" ]; then
        echo "Pure C++ Tests:" >> ${RESULTS_DIR}/summary_report.txt
        grep "✓" ${RESULTS_DIR}/pure_cpp_results.log >> ${RESULTS_DIR}/summary_report.txt
        echo "" >> ${RESULTS_DIR}/summary_report.txt
    fi
    
    if [ -f "${RESULTS_DIR}/hls_synthesis.log" ]; then
        echo "HLS Synthesis Results:" >> ${RESULTS_DIR}/summary_report.txt
        grep -A 5 "Timing" ${RESULTS_DIR}/hls_synthesis.log >> ${RESULTS_DIR}/summary_report.txt 2>/dev/null || true
        echo "" >> ${RESULTS_DIR}/summary_report.txt
    fi
    
    print_status "Summary report generated: ${RESULTS_DIR}/summary_report.txt"
}

# Main execution
main() {
    echo ""
    print_status "Starting SW-QPS build and test process..."
    echo ""
    
    # Step 1: Pure C++ tests
    print_status "Step 1: Pure C++ Testing"
    compile_pure_cpp
    run_pure_cpp
    echo ""
    
    # Step 2: HLS Synthesis (if available)
    print_status "Step 2: HLS Synthesis"
    run_hls_synthesis
    echo ""
    
    # Step 3: Performance Analysis
    print_status "Step 3: Performance Analysis"
    run_performance_analysis
    echo ""
    
    # Step 4: Generate Reports
    print_status "Step 4: Report Generation"
    generate_reports
    echo ""
    
    print_status "========================================"
    print_status "SW-QPS build and test complete!"
    print_status "Results available in: ${RESULTS_DIR}/"
    print_status "========================================"
}

# Parse command line arguments
case "${1:-all}" in
    pure)
        compile_pure_cpp
        run_pure_cpp
        ;;
    hls)
        run_hls_synthesis
        ;;
    perf)
        run_performance_analysis
        ;;
    report)
        generate_reports
        ;;
    all)
        main
        ;;
    clean)
        print_status "Cleaning build artifacts..."
        rm -rf ${PROJECT_DIR}/sw_qps_project
        rm -rf ${PROJECT_DIR}/phase1_test
        rm -rf ${RESULTS_DIR}
        rm -f *.csv *.log
        print_status "Clean complete"
        ;;
    *)
        echo "Usage: $0 {pure|hls|perf|report|all|clean}"
        echo "  pure   - Run pure C++ tests only"
        echo "  hls    - Run HLS synthesis only"
        echo "  perf   - Run performance analysis only"
        echo "  report - Generate reports only"
        echo "  all    - Run complete flow (default)"
        echo "  clean  - Clean all build artifacts"
        exit 1
        ;;
esac
