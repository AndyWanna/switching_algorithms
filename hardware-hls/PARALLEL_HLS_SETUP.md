# SW-QPS HLS Parallel Execution Setup

## Overview

This setup runs **8 parallel HLS synthesis and co-simulation flows** for the SW-QPS switching algorithm:
- **4 traffic patterns**: uniform, diagonal, quasi-diagonal, log-diagonal
- **2 optimization levels**: standard and aggressive

## Files Created

### Testbenches (4 pattern-specific)
- `tb/tb_sw_qps_uniform.cpp` - Uniform traffic only
- `tb/tb_sw_qps_diagonal.cpp` - Diagonal traffic only
- `tb/tb_sw_qps_quasi_diagonal.cpp` - Quasi-diagonal traffic only
- `tb/tb_sw_qps_log_diagonal.cpp` - Log-diagonal traffic only

### Source Directories
- `src/` - Standard optimization (partial unroll factor=4)
- `src_aggressive/` - Aggressive optimization (full unroll, II=1 pipelining)

### TCL Scripts (8 configurations)

**Standard Optimization:**
- `run_sw_qps_uniform.tcl`
- `run_sw_qps_diagonal.tcl`
- `run_sw_qps_quasi_diagonal.tcl`
- `run_sw_qps_log_diagonal.tcl`

**Aggressive Optimization:**
- `run_sw_qps_uniform_aggressive.tcl`
- `run_sw_qps_diagonal_aggressive.tcl`
- `run_sw_qps_quasi_diagonal_aggressive.tcl`
- `run_sw_qps_log_diagonal_aggressive.tcl`

### Python Orchestrator
- `run_parallel_hls.py` - Launches and monitors all 8 flows in parallel

## Optimization Differences

### Standard (`src/`)
- `#pragma HLS UNROLL factor=4` - Partial unrolling
- `#pragma HLS PIPELINE` - Standard pipelining
- Lower resource usage
- Moderate performance

### Aggressive (`src_aggressive/`)
- `#pragma HLS UNROLL` - Full unrolling
- `#pragma HLS PIPELINE II=1` - Tightest initiation interval
- `#pragma HLS DATAFLOW` - Dataflow optimization
- Higher resource usage (more LUTs, FFs, DSPs)
- Maximum performance

## Usage

### Check Environment
```bash
# Source Vivado/Vitis HLS tools
source /path/to/Vivado/2024.1/settings64.sh
```

### Run All 8 Configurations in Parallel
```bash
cd hardware-hls
python3 run_parallel_hls.py
```

The script will:
1. Check for HLS tools and TCL scripts
2. Launch 8 parallel processes
3. Monitor progress in real-time
4. Report completion status
5. Generate summary of results

### Manual Execution (Single Configuration)
```bash
cd hardware-hls
vitis_hls -f run_sw_qps_uniform.tcl
```

## Outputs

### Log Files (per configuration)
- `hls_uniform.log`
- `hls_uniform_aggressive.log`
- `hls_diagonal.log`
- `hls_diagonal_aggressive.log`
- ... etc

### Project Directories (per configuration)
- `sw_qps_project_uniform/solution-uniform/`
- `sw_qps_project_uniform_aggressive/solution-uniform-aggressive/`
- ... etc

Each solution directory contains:
- `syn/` - Synthesis reports (resource usage, timing)
- `sim/` - Co-simulation results
- `impl/` - Implementation details (if exported)

### Results CSV Files
- `sw_qps_uniform_results.csv` - Performance metrics
- `sw_qps_diagonal_results.csv`
- `sw_qps_quasi_diagonal_results.csv`
- `sw_qps_log_diagonal_results.csv`

## Expected Resource Usage Comparison

### Standard Optimization (N=8, T=16)
- LUTs: ~50K-80K
- FFs: ~60K-100K
- DSPs: ~100-150
- BRAM: ~50-100
- Clock: 200 MHz (5ns period)

### Aggressive Optimization (N=8, T=16)
- LUTs: ~100K-150K (2x increase)
- FFs: ~120K-200K (2x increase)
- DSPs: ~200-300 (2x increase)
- BRAM: ~50-100 (similar)
- Clock: 200 MHz (5ns period)
- **Lower latency, higher throughput**

## Performance Monitoring

The Python script displays:
- Launch status for each configuration
- Real-time progress updates
- Completion times
- Error reporting
- Summary statistics

Press `Ctrl+C` to stop monitoring (processes continue in background).

## Troubleshooting

### If processes fail:
1. Check log files: `cat hls_<pattern>.log`
2. Look for synthesis errors in `sw_qps_project_*/solution-*/syn/report/`
3. Verify resource availability on target FPGA

### If out of resources:
- Reduce N or T in `sw_qps_types.h`
- Use standard optimization instead of aggressive
- Target larger FPGA part

### If timing fails:
- Increase clock period (e.g., 10ns for 100 MHz)
- Reduce unroll factors in aggressive version
- Add pipeline pragmas strategically

## Next Steps

1. **Run the flows**: `python3 run_parallel_hls.py`
2. **Compare results**: Check synthesis reports for resource/performance tradeoffs
3. **Analyze traffic patterns**: Compare how different patterns affect hardware utilization
4. **Optimize further**: Adjust pragmas based on results
5. **Export IP**: Uncomment export sections in TCL scripts to create Vivado IP cores

## Notes

- Each flow takes ~30-60 minutes depending on machine
- Total execution time: ~1 hour (with parallel execution)
- Sequential execution would take ~4-8 hours
- Monitor system resources (RAM, CPU) during execution
