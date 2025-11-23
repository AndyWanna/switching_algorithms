# SW-QPS Implementation - Conversation Summary

## Project Overview
Implemented a Sliding-Window Queue-Proportional Sampling (SW-QPS) algorithm for input-queued network switches based on the 2020 paper by Meng, Gong, and Xu. The goal was to create an HLS-synthesizable implementation that achieves 85-93% throughput with O(1) time complexity.

## What Was Built

### Phase 1: Initial Exploration
- Reviewed the existing HLS testbench files provided by the user
- Identified basic QPS sampler, types, utilities already implemented
- These were for testing synthesizability of core components

### Phase 2-5: Full Implementation
Created a complete SW-QPS implementation with:

1. **Core Modules**:
   - `input_port.h`: Manages Virtual Output Queues (VOQs) and generates proposals using QPS
   - `output_port.h`: Processes proposals and maintains switching calendar
   - `sliding_window.h`: Coordinates the overall algorithm

2. **Top-Level Design**:
   - `sw_qps_top.h`: Provides multiple interfaces (cycle-by-cycle, single-cycle, streaming)
   - Supports N=64 ports with T=16 time slot window
   - Implements knockout threshold of 3 proposals per output

3. **Testing Infrastructure**:
   - Pure C++ testbench for algorithm verification
   - HLS co-simulation testbench with traffic generation
   - Network simulator for throughput/delay analysis
   - Support for 4 traffic patterns: uniform, diagonal, quasi-diagonal, log-diagonal

4. **Build System**:
   - TCL scripts for HLS synthesis
   - Bash build script for automated testing
   - Makefile for network simulator
   - Python plotting scripts for results

## Known Bug Discovered

**Critical Issue**: The implementation reports more packets departed than arrived, causing throughput calculations to exceed 100%.

### Likely Root Causes:

1. **Proposal Routing Problem**: In `sliding_window.h`, the system tries to determine which output port a proposal targets by matching VOQ lengths, which is unreliable (multiple VOQs can have the same length).

2. **Missing Output Port ID**: The `Proposal` struct doesn't include which output port it's targeting, making routing ambiguous.

3. **VOQ State Synchronization**: The testbench maintains a local copy of VOQ state that may become desynchronized with the actual implementation.

4. **Packet Removal Logic**: Packets might be removed from VOQs without verifying they exist, or the same packet might be removed multiple times.

## Key Implementation Details

- **Algorithm**: Each cycle runs one SW-QPS iteration and graduates one matching
- **QPS**: Samples output ports with probability proportional to VOQ length  
- **FFA**: First-Fit Accept finds earliest mutual slot availability
- **Window**: Maintains T=16 matchings simultaneously, graduating one per cycle

## Files Structure
All implementation files are in `/mnt/user-data/outputs/`:
- `hardware-hls/src/`: Core implementation modules
- `hardware-hls/tb/`: Testbenches
- `sw_qps_simulator.*`: Network simulation
- Build scripts and documentation

## Next Steps for Debugging

1. Add output_id field to Proposal struct
2. Fix proposal routing in sliding_window.h
3. Add assertions to catch negative VOQ lengths
4. Ensure packet counting is consistent
5. Verify each packet is processed exactly once

The implementation is functionally complete but needs this critical bug fix before it can be synthesized and deployed on an FPGA.
