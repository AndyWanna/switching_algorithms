#ifndef SW_QPS_TYPES_H
#define SW_QPS_TYPES_H

#include <ap_fixed.h>
#include <hls_stream.h>

// ============================================================================
// CONFIGURATION PARAMETERS
// ============================================================================

#define N 64                    // Number of input/output ports
#define T 16                    // Window size (time slots)
#define MAX_VOQ_LEN 1024       // Maximum queue length
#define LOG2_MAX_VOQ 10        // log2(MAX_VOQ_LEN)
#define KNOCKOUT_THRESH 3      // Max proposals per output port
#define INVALID_PORT 127       // Marker for unmatched

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

// 7th bit just to mark invalid port - irl we will only have 6 bits
typedef ap_uint<7> port_id_t;           // Port ID: 0-63 (need 6 bits)
typedef ap_uint<LOG2_MAX_VOQ> queue_len_t;  // Queue length: 0-1023
typedef ap_uint<T> avail_bitmap_t;      // Availability: 16 bits
typedef ap_uint<4> slot_id_t;           // Time slot: 0-15
typedef ap_uint<32> random_t;           // Random number

// ============================================================================
// MESSAGE STRUCTURES
// ============================================================================

// Proposal message: Input -> Output
struct Proposal {
    port_id_t input_id;         // Which input port sent this
    queue_len_t voq_len;        // Length of corresponding VOQ
    avail_bitmap_t availability; // Which slots input is free
    bool valid;                 // Is this proposal valid?
    
    Proposal() : input_id(0), voq_len(0), availability(0), valid(false) {}
};

// Accept message: Output -> Input
struct Accept {
    port_id_t output_id;        // Which output port sent this
    slot_id_t time_slot;        // Which slot was accepted
    bool valid;                 // Is this acceptance valid?
    
    Accept() : output_id(0), time_slot(0), valid(false) {}
};

// ============================================================================
// STATE STRUCTURES
// ============================================================================

// VOQ state at each input port
struct VOQState {
    queue_len_t lengths[N];     // Length of each VOQ
    queue_len_t sum;            // Total packets (sum of all VOQs)
    avail_bitmap_t availability; // Which slots this input is free
    
    VOQState() : sum(0), availability(~avail_bitmap_t(0)) {
        #pragma HLS ARRAY_PARTITION variable=lengths complete
        for (int i = 0; i < N; i++) {
            lengths[i] = 0;
        }
    }
};

// Calendar for one output port
struct Calendar {
    port_id_t schedule[T];      // Which input for each slot
    avail_bitmap_t availability; // Which slots are still free
    
    Calendar() : availability(~avail_bitmap_t(0)) {
        #pragma HLS ARRAY_PARTITION variable=schedule complete
        for (int i = 0; i < T; i++) {
            schedule[i] = INVALID_PORT;
        }
    }
};

// Complete sliding window state
struct SlidingWindow {
    Calendar calendars[N];      // One calendar per output port
    slot_id_t senior_idx;       // Which slot graduates this cycle
    
    SlidingWindow() : senior_idx(0) {
        #pragma HLS ARRAY_PARTITION variable=calendars complete
    }
};

#endif // SW_QPS_TYPES_H