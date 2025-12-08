#include "sw_qps_top.h"

void sw_qps_top(
    PacketArrival arrivals[N],
    bool run_iteration,
    bool graduate,
    port_id_t matching[N],
    ap_uint<8>& matching_size,
    bool& system_stable,
    bool reset
) {
    #pragma HLS INTERFACE mode=ap_ctrl_hs port=return
    #pragma HLS INTERFACE mode=ap_none port=arrivals
    #pragma HLS INTERFACE mode=ap_none port=run_iteration
    #pragma HLS INTERFACE mode=ap_none port=graduate
    #pragma HLS INTERFACE mode=ap_none port=matching
    #pragma HLS INTERFACE mode=ap_none port=matching_size
    #pragma HLS INTERFACE mode=ap_none port=system_stable
    #pragma HLS INTERFACE mode=ap_none port=reset
    #pragma HLS ARRAY_PARTITION variable=arrivals complete
    #pragma HLS ARRAY_PARTITION variable=matching complete

    static SlidingWindowManager sw_manager;

    if (reset) {
        sw_manager.initialize();
        for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL
            matching[i] = INVALID_PORT;
        }
        matching_size = 0;
        system_stable = true;
        return;
    }

    for (int i = 0; i < N; i++) {
        #pragma HLS UNROLL factor=4
        if (arrivals[i].valid) {
            sw_manager.addPacket(arrivals[i].input_port, arrivals[i].output_port);
        }
    }

    if (run_iteration) {
        sw_manager.runIteration();
    }

    if (graduate) {
        MatchingResult result = sw_manager.graduateMatching();
        for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL
            matching[i] = result.matching[i];
        }
        matching_size = result.matching_size;
    } else {
        for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL
            matching[i] = INVALID_PORT;
        }
        matching_size = 0;
    }

    system_stable = sw_manager.isStable();
}

void sw_qps_single_cycle(
    queue_len_t voq_state[N][N],
    ap_uint<4> num_iterations,
    port_id_t matching[N],
    ap_uint<8>& matching_size,
    bool reset
) {
    #pragma HLS INTERFACE mode=ap_ctrl_hs port=return
    #pragma HLS INTERFACE mode=ap_none port=voq_state
    #pragma HLS INTERFACE mode=ap_none port=num_iterations
    #pragma HLS INTERFACE mode=ap_none port=matching
    #pragma HLS INTERFACE mode=ap_none port=matching_size
    #pragma HLS INTERFACE mode=ap_none port=reset
    #pragma HLS ARRAY_PARTITION variable=voq_state dim=1 complete
    #pragma HLS ARRAY_PARTITION variable=matching complete

    static SlidingWindowManager sw_manager;

    if (reset) {
        sw_manager.initialize();
        for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL
            matching[i] = INVALID_PORT;
        }
        matching_size = 0;
        return;
    }

    sw_manager.loadTrafficMatrix(voq_state);

    for (int iter = 0; iter < num_iterations && iter < T; iter++) {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=16
        sw_manager.runIteration();
    }

    MatchingResult result = sw_manager.graduateMatching();
    for (int i = 0; i < N; i++) {
        #pragma HLS UNROLL
        matching[i] = result.matching[i];
    }
    matching_size = result.matching_size;
}

void sw_qps_stream(
    hls::stream<PacketArrival>& arrival_stream,
    hls::stream<bool>& control_stream,
    hls::stream<MatchingResult>& matching_stream,
    bool run,
    bool reset
) {
    #pragma HLS INTERFACE mode=ap_ctrl_hs port=return
    #pragma HLS INTERFACE mode=axis port=arrival_stream
    #pragma HLS INTERFACE mode=axis port=control_stream
    #pragma HLS INTERFACE mode=axis port=matching_stream
    #pragma HLS INTERFACE mode=ap_none port=run
    #pragma HLS INTERFACE mode=ap_none port=reset

    static SlidingWindowManager sw_manager;
    static int cycle_count = 0;

    if (reset) {
        sw_manager.initialize();
        cycle_count = 0;
        return;
    }

    if (!run) {
        return;
    }

    while (!arrival_stream.empty()) {
        PacketArrival arrival = arrival_stream.read();
        if (arrival.valid) {
            sw_manager.addPacket(arrival.input_port, arrival.output_port);
        }
    }

    if (!control_stream.empty()) {
        bool do_iteration = control_stream.read();
        if (do_iteration) {
            sw_manager.runIteration();
        }
    }

    cycle_count++;
    if (cycle_count >= 1) {
        MatchingResult result = sw_manager.graduateMatching();
        matching_stream.write(result);
        cycle_count = 0;
    }
}
