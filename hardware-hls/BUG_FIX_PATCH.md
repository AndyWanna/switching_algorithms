# SW-QPS Critical Bug Fix Patch

## Bug: Throughput > 100% due to incorrect proposal routing

### Root Cause
The `Proposal` struct doesn't include which output port is being targeted, causing ambiguous routing when multiple VOQs have the same length.

## Required Changes

### 1. Fix `sw_qps_types.h`
```cpp
// Around line 33, modify the Proposal struct:
struct Proposal {
    port_id_t input_id;         // Which input port sent this
    port_id_t output_id;        // ADD THIS: Which output port this targets
    queue_len_t voq_len;        // Length of corresponding VOQ  
    avail_bitmap_t availability; // Which slots input is free
    bool valid;                 // Is this proposal valid?
    
    Proposal() : input_id(0), output_id(INVALID_PORT), voq_len(0), 
                 availability(0), valid(false) {}
};
```

### 2. Fix `input_port.h`
```cpp
// Around line 70, modify generateProposal():
Proposal generateProposal() {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE
    
    Proposal prop;
    prop.input_id = port_id;
    prop.availability = availability;
    prop.valid = false;
    prop.output_id = INVALID_PORT;  // ADD THIS
    
    // Update LFSR for randomness
    lfsr_state = lfsr_next(lfsr_state);
    
    // Sample output port using QPS
    port_id_t sampled_output = QPSSampler::sample(voq_state, lfsr_state);
    
    if (sampled_output != INVALID_PORT && voq_state.lengths[sampled_output] > 0) {
        prop.output_id = sampled_output;  // ADD THIS: Set the output port!
        prop.voq_len = voq_state.lengths[sampled_output];
        prop.valid = true;
        return prop;
    }
    
    prop.valid = false;
    return prop;
}
```

### 3. Fix `sliding_window.h`
```cpp
// Around line 90-120, fix the proposal routing in runIteration():

// Phase 1: Generate proposals from all input ports
PROPOSE_PHASE: for (int i = 0; i < N; i++) {
    #pragma HLS UNROLL factor=4
    
    Proposal prop = input_ports[i].generateProposal();
    
    if (prop.valid) {
        // NOW WE HAVE THE OUTPUT PORT DIRECTLY!
        port_id_t target_output = prop.output_id;  // USE THIS
        
        if (target_output != INVALID_PORT && target_output < N) {
            int idx = num_proposals_per_output[target_output];
            if (idx < N) {
                proposals_per_output[target_output][idx] = prop;
                num_proposals_per_output[target_output]++;
            }
        }
    }
}

// Later in the accept phase, around line 140:
// Find which input this accept is for
for (int k = 0; k < num_proposals_per_output[i]; k++) {
    // FIX: Use the input_id from the proposal directly
    if (proposals_per_output[i][k].valid) {
        port_id_t input_id = proposals_per_output[i][k].input_id;
        // Now route the accept to the correct input
        if (accepts[j].valid && input_id < N) {
            input_ports[input_id].processAccept(accepts[j]);
        }
    }
}
```

### 4. Fix `input_port.h` - Add safety check
```cpp
// Around line 85, make removePacket safer:
void removePacket(port_id_t output_port) {
    #pragma HLS INLINE
    if (output_port < N && voq_state.lengths[output_port] > 0) {
        voq_state.lengths[output_port]--;
        voq_state.sum--;
    } else {
        // Log error or assert in debug mode
        #ifndef __SYNTHESIS__
        if (output_port < N && voq_state.lengths[output_port] == 0) {
            // Trying to remove packet from empty VOQ!
            assert(false && "Removing packet from empty VOQ!");
        }
        #endif
    }
}
```

### 5. Fix testbench packet counting (`tb_sw_qps_hls.cpp`)
```cpp
// Around line 180, fix the departure counting:
// Graduate matching
sw_qps_top(arrivals, false, true, matching, matching_size, stable, false);

// Update local VOQ state based on departures
int actual_departures = 0;
for (int out = 0; out < N; out++) {
    if (matching[out] != INVALID_PORT) {
        int in = matching[out];
        // CRITICAL: Only count if packet exists!
        if (voq_lengths[in][out] > 0) {
            voq_lengths[in][out]--;
            actual_departures++;
        } else {
            // ERROR: Trying to depart non-existent packet!
            cout << "ERROR: Matching claims packet from empty VOQ[" 
                 << in << "][" << out << "]" << endl;
        }
    }
}

// Record statistics after warmup
if (cycle >= warmup_time) {
    monitor.recordArrivals(arrival_count);
    monitor.recordMatching(actual_departures);  // Use actual, not matching_size!
    monitor.total_cycles++;
}
```

## Quick Test After Fix

Add this simple test to verify the fix:

```cpp
void test_packet_conservation() {
    cout << "Testing packet conservation..." << endl;
    
    // Reset
    PacketArrival arrivals[N];
    port_id_t matching[N];
    ap_uint<8> matching_size;
    bool stable;
    
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, true);
    
    // Add exactly 10 packets
    for (int i = 0; i < 10; i++) {
        arrivals[i].input_port = i;
        arrivals[i].output_port = i;
        arrivals[i].valid = true;
    }
    for (int i = 10; i < N; i++) {
        arrivals[i].valid = false;
    }
    
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, false);
    
    // Clear arrivals
    for (int i = 0; i < N; i++) arrivals[i].valid = false;
    
    // Run many cycles
    int total_departed = 0;
    for (int cycle = 0; cycle < 100; cycle++) {
        sw_qps_top(arrivals, true, false, matching, matching_size, stable, false);
        sw_qps_top(arrivals, false, true, matching, matching_size, stable, false);
        
        // Count actual departures (not just matching_size)
        for (int out = 0; out < N; out++) {
            if (matching[out] != INVALID_PORT) {
                total_departed++;
            }
        }
    }
    
    cout << "Packets added: 10" << endl;
    cout << "Packets departed: " << total_departed << endl;
    
    assert(total_departed <= 10);
    cout << "✓ Packet conservation test passed!" << endl;
}
```

## Expected Result After Fix
- Throughput should never exceed 100%
- Total departures ≤ Total arrivals
- No negative VOQ lengths
- Each packet counted exactly once

## To Apply the Fix:
1. Make the changes above to the specified files
2. Recompile the testbench
3. Run the packet conservation test
4. Run full simulation - throughput should now be < 100%
