#ifndef SW_QPS_TOP_H
#define SW_QPS_TOP_H

#include "sw_qps_types.h"
#include "sliding_window.h"

/*
 * ============================================================================
 * SW-QPS TOP MODULE
 * ============================================================================
 * 
 * Top-level module for HLS synthesis
 * Implements complete SW-QPS switching algorithm
 * 
 * Interface:
 *   - Input: Packet arrivals
 *   - Output: Matched pairs for switching
 *   - Control: Run iterations and graduate matchings
 */

// Simplified packet arrival interface
struct PacketArrival {
    port_id_t input_port;
    port_id_t output_port;
    bool valid;
    
    PacketArrival() : input_port(0), output_port(0), valid(false) {}
};

void sw_qps_top(
    PacketArrival arrivals[N],
    bool run_iteration,
    bool graduate,
    port_id_t matching[N],
    ap_uint<8>& matching_size,
    bool& system_stable,
    bool reset
);

void sw_qps_single_cycle(
    queue_len_t voq_state[N][N],
    ap_uint<4> num_iterations,
    port_id_t matching[N],
    ap_uint<8>& matching_size,
    bool reset
);

void sw_qps_stream(
    hls::stream<PacketArrival>& arrival_stream,
    hls::stream<bool>& control_stream,
    hls::stream<MatchingResult>& matching_stream,
    bool run,
    bool reset
);

#endif // SW_QPS_TOP_H
