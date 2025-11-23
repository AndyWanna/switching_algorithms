# SW-QPS Implementation Debugging Task

## Context
You are debugging a Sliding-Window Queue-Proportional Sampling (SW-QPS) implementation for an input-queued network switch. The implementation is in HLS C++ and includes a complete testbench. The algorithm is based on the paper "Sliding-Window QPS (SW-QPS): A Perfect Parallel Iterative Switching Algorithm for Input-Queued Switches" by Meng, Gong, and Xu (2020).

## Critical Bug
**Issue**: The simulation reports MORE packets departed than arrived, leading to impossible throughput values over 100%. This is happening across all traffic patterns and load levels.

**Example symptoms**:
- Total arrivals: 10,000 packets
- Total departures: 12,000+ packets (impossible!)
- Normalized throughput: > 100%

## System Architecture Overview

### Key Components:
1. **N=64 input ports**, each with N Virtual Output Queues (VOQs)
2. **N=64 output ports**, each maintaining a calendar of T=16 time slots
3. **Sliding window** of T=16 matchings under computation
4. **QPS sampling** for proposal generation
5. **First-Fit Accept (FFA)** for accepting proposals

### Algorithm Flow per Cycle:
1. Generate packet arrivals based on traffic pattern
2. Add packets to VOQs at input ports
3. Run SW-QPS iteration (propose-accept phases)
4. Graduate senior matching (slot 0 of window)
5. Remove matched packets from VOQs
6. Shift window forward

## Suspected Bug Locations

### PRIMARY SUSPECTS:

1. **File: `hardware-hls/src/sliding_window.h`** - `graduateMatching()` function
   - Might be removing packets that don't exist in VOQs
   - Check the coordination between input and output ports

2. **File: `hardware-hls/src/input_port.h`** - `removePacket()` function
   - May not be checking if packets actually exist before removal
   - Could be decrementing below zero

3. **File: `hardware-hls/tb/tb_sw_qps_hls.cpp`** - Traffic generation and packet counting
   - Double-counting departures
   - VOQ state tracking mismatch

4. **File: `hardware-hls/src/sliding_window.h`** - `runIteration()` function
   - Proposal routing logic might be incorrect
   - Accept messages might not be routed to correct input ports

## Debugging Strategy

### Step 1: Add Assertions and Checks
```cpp
// In input_port.h - removePacket()
void removePacket(port_id_t output_port) {
    #pragma HLS INLINE
    assert(output_port < N);
    assert(voq_state.lengths[output_port] > 0);  // ADD THIS
    if (output_port < N && voq_state.lengths[output_port] > 0) {
        voq_state.lengths[output_port]--;
        voq_state.sum--;
    }
    // Add else clause to log error
}
```

### Step 2: Fix VOQ State Tracking
The testbench maintains a local copy of VOQ state that might get out of sync. Check:
- File: `tb_sw_qps_hls.cpp`, around line 150-180
- The local `voq_lengths[N][N]` array must match actual VOQ state

### Step 3: Fix Matching/Departure Logic
```cpp
// In tb_sw_qps_hls.cpp - the departure counting logic
for (int out = 0; out < N; out++) {
    if (matching[out] != INVALID_PORT) {
        int in = matching[out];
        // CRITICAL: Only count as departure if packet exists!
        if (voq_lengths[in][out] > 0) {
            voq_lengths[in][out]--;
            // Only count departure here
            departures++;
        }
    }
}
```

### Step 4: Fix Sliding Window Coordination
The issue might be in how the sliding window manager coordinates between input and output ports:

**File: `sliding_window.h`** - Check the `graduateMatching()` function:
- Ensure input ports are only notified once per graduation
- Verify that the same packet isn't being removed multiple times
- Check that input_id tracking in proposals is correct

## Quick Test to Isolate Bug

Create a minimal test case:
```cpp
// Add this test to tb_sw_qps_hls.cpp
void test_single_packet() {
    // Reset system
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, true);
    
    // Add exactly ONE packet
    arrivals[0].input_port = 0;
    arrivals[0].output_port = 0;
    arrivals[0].valid = true;
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, false);
    
    // Clear arrivals
    arrivals[0].valid = false;
    
    // Run T iterations and T graduations
    int total_departed = 0;
    for (int i = 0; i < T*2; i++) {
        sw_qps_top(arrivals, true, false, matching, matching_size, stable, false);
        sw_qps_top(arrivals, false, true, matching, matching_size, stable, false);
        total_departed += matching_size;
    }
    
    // Should have exactly 1 departure, not more!
    assert(total_departed <= 1);
}
```

## Root Cause Hints

### Most Likely Issue:
The `sliding_window.h` file has a complex proposal routing mechanism that might be broken. Look at lines in `runIteration()` where it tries to determine which output port a proposal is for:

```cpp
// This logic is suspicious (around line 100-110 in sliding_window.h):
for (int j = 0; j < N; j++) {
    if (input_ports[i].getVOQLength(j) == prop.voq_len) {
        target_output = j;
        break;  // Problem: multiple VOQs might have same length!
    }
}
```

### Critical Fix Needed:
The proposal should include the target output port! Modify the `Proposal` struct in `sw_qps_types.h`:
```cpp
struct Proposal {
    port_id_t input_id;
    port_id_t output_id;  // ADD THIS!
    queue_len_t voq_len;
    avail_bitmap_t availability;
    bool valid;
};
```

Then update `input_port.h` to include the output port in proposals.

## Files to Review (in order):

1. **`hardware-hls/tb/tb_sw_qps_hls.cpp`** - Lines 150-200 (departure counting)
2. **`hardware-hls/src/sliding_window.h`** - Lines 70-150 (runIteration and proposal routing)
3. **`hardware-hls/src/input_port.h`** - Lines 50-90 (generateProposal and removePacket)
4. **`hardware-hls/src/sw_qps_types.h`** - Proposal struct definition

## Validation After Fix

After fixing, verify:
1. Total departures ≤ Total arrivals (always!)
2. Normalized throughput ≤ 1.0
3. VOQ lengths never go negative
4. Each packet is counted exactly once

## Command to Test:
```bash
cd hardware-hls
vitis_hls -f run_sw_qps.tcl
```

Look for any output showing throughput > 100% or negative VOQ lengths.

Good luck debugging! The issue is most likely in the proposal routing logic in `sliding_window.h` where proposals need to track their target output port explicitly.
