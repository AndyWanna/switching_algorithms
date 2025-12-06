#!/usr/bin/env python3
"""
============================================================================
SW-QPS HLS Parallel Execution Script
============================================================================

Executes 4 HLS synthesis and co-simulation flows in parallel, one for each
traffic pattern: uniform, diagonal, quasi-diagonal, and log-diagonal.

Each flow runs in a separate background process with output redirected to
pattern-specific log files.

Usage:
    python3 run_parallel_hls.py

The script will:
1. Launch 4 parallel vitis_hls processes
2. Monitor their progress
3. Report completion status
4. Aggregate results from all patterns
"""

import subprocess
import os
import time
import sys
from datetime import datetime
from pathlib import Path

# Configuration
PATTERNS = ["uniform", "diagonal", "quasi_diagonal", "log_diagonal"]
OPTIMIZATIONS = ["", "_aggressive"]  # Standard and aggressive optimization levels
HLS_COMMAND = "vitis_hls"  # Adjust if needed (might be vivado_hls on older versions)
WORKING_DIR = Path(__file__).parent.absolute()

class HLSRunner:
    """Manages parallel execution of HLS flows"""
    
    def __init__(self):
        self.processes = {}
        self.start_times = {}
        self.log_files = {}
        self.configurations = []  # List of (pattern, optimization) tuples
        
    def launch_pattern(self, pattern, optimization=""):
        """Launch HLS flow for a specific pattern and optimization level"""
        
        # Create config identifier
        config_name = f"{pattern}{optimization}"
        
        # Map pattern names to script names
        script_name = f"run_sw_qps_{pattern}{optimization}.tcl"
        log_file = WORKING_DIR / f"hls_{pattern}{optimization}.log"
        
        opt_label = "AGGRESSIVE" if optimization else "STANDARD"
        print(f"[{self._timestamp()}] Launching {pattern} ({opt_label}) flow...")
        print(f"  Script: {script_name}")
        print(f"  Log: {log_file}")
        
        # Open log file
        log_handle = open(log_file, 'w')
        self.log_files[config_name] = log_handle
        
        # Launch process
        try:
            process = subprocess.Popen(
                [HLS_COMMAND, "-f", script_name],
                cwd=WORKING_DIR,
                stdout=log_handle,
                stderr=subprocess.STDOUT,
                universal_newlines=True
            )
            
            self.processes[config_name] = process
            self.start_times[config_name] = time.time()
            self.configurations.append((pattern, optimization))
            
            print(f"  ✓ Started (PID: {process.pid})")
            return True
            
        except FileNotFoundError:
            print(f"  ✗ Error: {HLS_COMMAND} not found!")
            print(f"    Make sure Vitis HLS is installed and in PATH")
            log_handle.close()
            return False
        except Exception as e:
            print(f"  ✗ Error launching: {e}")
            log_handle.close()
            return False
    
    def launch_all(self):
        """Launch all pattern flows"""
        print("=" * 70)
        print("SW-QPS HLS PARALLEL EXECUTION")
        print("=" * 70)
        print(f"Working directory: {WORKING_DIR}")
        
        total_configs = len(PATTERNS) * len(OPTIMIZATIONS)
        print(f"Launching {total_configs} parallel HLS flows...")
        print(f"  Patterns: {', '.join(PATTERNS)}")
        print(f"  Optimizations: Standard, Aggressive")
        print()
        
        success_count = 0
        for optimization in OPTIMIZATIONS:
            for pattern in PATTERNS:
                if self.launch_pattern(pattern, optimization):
                    success_count += 1
                time.sleep(0.5)  # Stagger launches slightly
        
        print()
        if success_count == 0:
            print("✗ Failed to launch any processes!")
            return False
        elif success_count < total_configs:
            print(f"⚠ Warning: Only {success_count}/{total_configs} processes launched")
        else:
            print(f"✓ All {success_count} processes launched successfully")
        
        print()
        return success_count > 0
    
    def monitor(self):
        """Monitor running processes"""
        print("=" * 70)
        print("MONITORING PROCESSES")
        print("=" * 70)
        print("Press Ctrl+C to stop monitoring (processes will continue)")
        print()
        
        try:
            while self.processes:
                # Check each process
                completed = []
                
                for pattern, process in self.processes.items():
                    returncode = process.poll()
                    
                    if returncode is not None:
                        # Process completed
                        elapsed = time.time() - self.start_times[pattern]
                        elapsed_str = self._format_duration(elapsed)
                        
                        if returncode == 0:
                            print(f"[{self._timestamp()}] ✓ {pattern:20s} COMPLETED ({elapsed_str})")
                        else:
                            print(f"[{self._timestamp()}] ✗ {pattern:20s} FAILED (exit code: {returncode}, {elapsed_str})")
                        
                        completed.append(pattern)
                
                # Remove completed processes
                for pattern in completed:
                    del self.processes[pattern]
                    self.log_files[pattern].close()
                
                if self.processes:
                    # Still running - show status
                    running = list(self.processes.keys())
                    elapsed_times = [time.time() - self.start_times[p] for p in running]
                    max_elapsed = max(elapsed_times)
                    
                    print(f"[{self._timestamp()}] Running: {', '.join(running)} "
                          f"(longest: {self._format_duration(max_elapsed)})", end='\r')
                    
                    time.sleep(10)  # Check every 10 seconds
                else:
                    print("\n")
                    break
                    
        except KeyboardInterrupt:
            print("\n\n⚠ Monitoring interrupted by user")
            print(f"  {len(self.processes)} process(es) still running in background:")
            for pattern, process in self.processes.items():
                print(f"    - {pattern} (PID: {process.pid})")
            print("\n  They will continue running. Check log files for progress.")
    
    def summarize(self):
        """Print summary of results"""
        print("\n" + "=" * 70)
        print("EXECUTION SUMMARY")
        print("=" * 70)
        
        # List all log files
        print("\nLog files:")
        for optimization in OPTIMIZATIONS:
            for pattern in PATTERNS:
                config_name = f"{pattern}{optimization}"
                log_file = WORKING_DIR / f"hls_{config_name}.log"
                if log_file.exists():
                    size = log_file.stat().st_size / 1024  # KB
                    print(f"  - {log_file.name:40s} ({size:8.1f} KB)")
        
        # List results CSV files
        print("\nResults files:")
        for pattern in PATTERNS:
            csv_file = WORKING_DIR / f"sw_qps_{pattern}_results.csv"
            if csv_file.exists():
                print(f"  ✓ {csv_file.name}")
            else:
                print(f"  ✗ {csv_file.name} (not found)")
        
        # List project directories
        print("\nProject directories:")
        for optimization in OPTIMIZATIONS:
            opt_suffix = "_aggressive" if optimization else ""
            for pattern in PATTERNS:
                proj_dir = WORKING_DIR / f"sw_qps_project_{pattern}{opt_suffix}"
                if proj_dir.exists():
                    print(f"  ✓ {proj_dir.name}/")
                else:
                    print(f"  ✗ {proj_dir.name}/ (not found)")
        
        print("\n" + "=" * 70)
        print("DONE")
        print("=" * 70)
    
    def _timestamp(self):
        """Get formatted timestamp"""
        return datetime.now().strftime("%H:%M:%S")
    
    def _format_duration(self, seconds):
        """Format duration in human-readable format"""
        if seconds < 60:
            return f"{seconds:.0f}s"
        elif seconds < 3600:
            mins = seconds / 60
            return f"{mins:.1f}m"
        else:
            hours = seconds / 3600
            return f"{hours:.1f}h"


def check_environment():
    """Check if HLS tools are available"""
    print("Checking environment...")
    
    # Check for vitis_hls or vivado_hls
    hls_found = False
    for cmd in ["vitis_hls", "vivado_hls"]:
        try:
            result = subprocess.run([cmd, "-version"], 
                                   capture_output=True, 
                                   timeout=5)
            if result.returncode == 0:
                print(f"✓ Found {cmd}")
                hls_found = True
                break
        except (FileNotFoundError, subprocess.TimeoutExpired):
            continue
    
    if not hls_found:
        print("✗ Error: No HLS tool found (vitis_hls or vivado_hls)")
        print("  Please source Vivado/Vitis settings first:")
        print("  source /path/to/Vivado/<version>/settings64.sh")
        return False
    
    # Check for required TCL scripts
    print("\nChecking TCL scripts...")
    all_found = True
    for optimization in OPTIMIZATIONS:
        for pattern in PATTERNS:
            config_name = f"{pattern}{optimization}"
            script = WORKING_DIR / f"run_sw_qps_{config_name}.tcl"
            if script.exists():
                print(f"✓ {script.name}")
            else:
                print(f"✗ {script.name} not found!")
                all_found = False
    
    if not all_found:
        return False
    
    print()
    return True


def main():
    """Main execution"""
    
    # Check environment
    if not check_environment():
        print("\n⚠ Environment check failed!")
        print("Please fix the issues above before running.")
        sys.exit(1)
    
    # Create runner and launch
    runner = HLSRunner()
    
    if not runner.launch_all():
        print("\n✗ Failed to launch HLS flows!")
        sys.exit(1)
    
    # Monitor execution
    runner.monitor()
    
    # Print summary
    runner.summarize()


if __name__ == "__main__":
    main()
