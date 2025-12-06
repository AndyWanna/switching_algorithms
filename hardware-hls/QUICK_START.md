# Quick Start Guide - Parallel HLS Execution

## What Was Created

### 8 HLS Configurations
- **4 traffic patterns** × **2 optimization levels** = **8 parallel synthesis flows**

| Pattern         | Standard                  | Aggressive                          |
|----------------|---------------------------|-------------------------------------|
| Uniform        | run_sw_qps_uniform.tcl    | run_sw_qps_uniform_aggressive.tcl   |
| Diagonal       | run_sw_qps_diagonal.tcl   | run_sw_qps_diagonal_aggressive.tcl  |
| Quasi-Diagonal | run_sw_qps_quasi_diagonal.tcl | run_sw_qps_quasi_diagonal_aggressive.tcl |
| Log-Diagonal   | run_sw_qps_log_diagonal.tcl | run_sw_qps_log_diagonal_aggressive.tcl |

### Optimization Levels

**Standard (`src/`):**
- Partial loop unrolling (factor=4)
- Balanced resource usage
- Good for initial testing

**Aggressive (`src_aggressive/`):**
- Full loop unrolling
- Tighter pipelining (II=1)
- Maximum performance, higher resources

## Run All 8 Configurations in Parallel

```bash
cd /usr/scratch/awanna3/ssched_simulator/hardware-hls
python3 run_parallel_hls.py
```

## What It Does

1. ✓ Checks for vitis_hls/vivado_hls
2. ✓ Launches 8 parallel processes
3. ✓ Monitors progress in real-time
4. ✓ Reports completion status
5. ✓ Generates summary

## Expected Runtime

- **Parallel execution**: ~60-90 minutes (all 8 at once)
- **Sequential execution**: ~8-12 hours (one at a time)

## Output Files

### Logs (8 files)
```
hls_uniform.log
hls_uniform_aggressive.log
hls_diagonal.log
hls_diagonal_aggressive.log
hls_quasi_diagonal.log
hls_quasi_diagonal_aggressive.log
hls_log_diagonal.log
hls_log_diagonal_aggressive.log
```

### Projects (8 directories)
```
sw_qps_project_uniform/
sw_qps_project_uniform_aggressive/
sw_qps_project_diagonal/
sw_qps_project_diagonal_aggressive/
... etc
```

### Results (4 CSV files)
```
sw_qps_uniform_results.csv
sw_qps_diagonal_results.csv
sw_qps_quasi_diagonal_results.csv
sw_qps_log_diagonal_results.csv
```

## Monitoring Progress

While running, you'll see:
```
[12:34:56] Running: uniform, diagonal, ... (longest: 45.3m)
[12:35:00] ✓ uniform COMPLETED (43.2m)
[12:35:05] ✓ diagonal COMPLETED (45.1m)
...
```

## Checking Results

### Resource Usage
```bash
# View synthesis report for uniform (standard)
less sw_qps_project_uniform/solution-uniform/syn/report/sw_qps_top_csynth.rpt

# View synthesis report for uniform (aggressive)
less sw_qps_project_uniform_aggressive/solution-uniform-aggressive/syn/report/sw_qps_top_csynth.rpt
```

### Compare Optimizations
```bash
# Extract resource usage from all reports
for proj in sw_qps_project_*/solution-*/syn/report/*_csynth.rpt; do
    echo "=== $proj ==="
    grep -A 10 "== Utilization Estimates" "$proj" | head -20
done
```

## Troubleshooting

### Process failed?
```bash
# Check the log file
tail -100 hls_uniform.log
```

### Out of memory?
- Reduce number of parallel jobs
- Run in batches (4 at a time)
- Close other applications

### Timing not met?
- Check synthesis reports
- May need to relax clock constraint
- Consider less aggressive optimization

## Next Steps

After completion:
1. Compare standard vs aggressive resource usage
2. Analyze performance across traffic patterns
3. Identify best configuration for your FPGA
4. Export IP cores for Vivado integration

## Manual Execution (if needed)

Run single configuration:
```bash
vitis_hls -f run_sw_qps_uniform.tcl
```

Run subset:
```bash
# Standard only
vitis_hls -f run_sw_qps_uniform.tcl &
vitis_hls -f run_sw_qps_diagonal.tcl &
wait

# Aggressive only
vitis_hls -f run_sw_qps_uniform_aggressive.tcl &
vitis_hls -f run_sw_qps_diagonal_aggressive.tcl &
wait
```
