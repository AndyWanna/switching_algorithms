#ifndef INPUT_PORT_H
#define INPUT_PORT_H

#include "sw_qps_types.h"
#include "qps_sampler.cpp"
#include "utils.h"
#include <hls_stream.h>

/*
 * ============================================================================
 * INPUT PORT MODULE
 * ============================================================================
 * 
 * Manages VOQs and generates proposals using QPS sampling
 * Each input port:
 *   1. Maintains N VOQs (one per output port)
 *   2. Samples output ports using QPS
 *   3. Generates proposals with availability bitmap
 *   4. Processes accepts and updates availability
 */

class InputPort {
private:
    port_id_t port_id;
    VOQState voq_state;
    avail_bitmap_t availability;  // Which time slots are available
    port_id_t schedule[T];        // Which output port for each slot
    random_t lfsr_state;          // Local LFSR for randomness

public:
    InputPort() : port_id(0), availability(~avail_bitmap_t(0)), lfsr_state(0) {
        #pragma HLS ARRAY_PARTITION variable=schedule complete
        for (int i = 0; i < T; i++) {
            #pragma HLS UNROLL
            schedule[i] = INVALID_PORT;
        }
    }
    
    void initialize(port_id_t id, random_t seed) {
        #pragma HLS INLINE
        port_id = id;
        lfsr_state = seed + id;  // Different seed per port
        availability = ~avail_bitmap_t(0);  // All slots initially available

        // Initialize schedule
        for (int i = 0; i < T; i++) {
            #pragma HLS UNROLL
            schedule[i] = INVALID_PORT;
        }

        // Initialize VOQ state
        for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL factor=4
            voq_state.lengths[i] = 0;
        }
        voq_state.sum = 0;
        voq_state.availability = ~avail_bitmap_t(0);
    }
    
    /*
     * Add packet to VOQ
     */
    void addPacket(port_id_t output_port, queue_len_t num_packets = 1) {
        #pragma HLS INLINE
        if (output_port < N && voq_state.lengths[output_port] < MAX_VOQ_LEN) {
            voq_state.lengths[output_port] += num_packets;
            voq_state.sum += num_packets;
        }
    }
    
    /*
     * Remove packet from VOQ (when matched)
     */
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
    
    /*
     * Check if output is already matched in any slot
     */
    bool isOutputMatched(port_id_t output) const {
        #pragma HLS INLINE
        for (int i = 0; i < T; i++) {
            #pragma HLS UNROLL
            if (schedule[i] == output) {
                return true;
            }
        }
        return false;
    }

    /*
     * Generate proposal using QPS
     * Returns: Proposal with sampled output port and availability
     */
    Proposal generateProposal() {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE off

        Proposal prop;
        prop.input_id = port_id;
        prop.availability = availability;
        prop.valid = false;
        prop.output_id = INVALID_PORT;

        // Try up to N times to find an unmatched output with packets
        const int MAX_ATTEMPTS = N;
        for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=64

            // Update LFSR for randomness
            lfsr_state = lfsr_next(lfsr_state);

            // Sample output port using QPS
            port_id_t sampled_output = QPSSampler::sample(voq_state, lfsr_state);

            // Check if valid, has packets, and not already matched
            if (sampled_output != INVALID_PORT &&
                voq_state.lengths[sampled_output] > 0 &&
                !isOutputMatched(sampled_output)) {

                prop.output_id = sampled_output;
                prop.voq_len = voq_state.lengths[sampled_output];
                prop.valid = true;
                return prop;
            }

            // If all VOQs to unmatched outputs are empty, stop trying
            if (voq_state.sum == 0) break;
        }

        prop.valid = false;
        return prop;
    }
    
    /*
     * Process accept message from output port
     * CRITICAL: Immediately remove packet (virtual departure) when match is accepted
     */
    void processAccept(const Accept& accept) {
        #pragma HLS INLINE
        if (accept.valid && accept.time_slot < T) {
            // Mark slot as unavailable
            availability &= ~(avail_bitmap_t(1) << accept.time_slot);
            voq_state.availability = availability;

            // Record which output is matched in this slot
            schedule[accept.time_slot] = accept.output_id;

            // CRITICAL: Virtual departure - immediately remove packet from VOQ
            // This prevents VOQ underflow when the match graduates later
            if (accept.output_id < N && voq_state.lengths[accept.output_id] > 0) {
                voq_state.lengths[accept.output_id]--;
                voq_state.sum--;
            }
        }
    }
    
    /*
     * Graduate senior slot and shift window
     * NOTE: Packet was already removed during virtual departure (processAccept)
     */
    void graduateSlot(bool matched, port_id_t output_port) {
        #pragma HLS INLINE

        // NOTE: Packet removal happens in processAccept (virtual departure)
        // Do NOT remove packet here or we'll underflow the VOQ

        // Shift schedule array (move all slots forward)
        for (int i = 0; i < T-1; i++) {
            #pragma HLS UNROLL
            schedule[i] = schedule[i+1];
        }
        schedule[T-1] = INVALID_PORT;  // New junior slot is empty

        // Shift availability window (add new slot at end)
        availability = (availability >> 1) | (avail_bitmap_t(1) << (T-1));
        voq_state.availability = availability;
    }
    
    /*
     * Get current state (for monitoring)
     */
    queue_len_t getVOQLength(port_id_t output_port) const {
        #pragma HLS INLINE
        queue_len_t zero = 0;
        return (output_port < N) ? voq_state.lengths[output_port] : zero;
    }
    
    queue_len_t getTotalPackets() const {
        #pragma HLS INLINE
        return voq_state.sum;
    }
    
    avail_bitmap_t getAvailability() const {
        #pragma HLS INLINE
        return availability;
    }
    
    /*
     * Load traffic (for testing)
     */
    void loadTraffic(queue_len_t lengths[N]) {
        #pragma HLS INLINE off
        voq_state.sum = 0;
        for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL factor=4
            voq_state.lengths[i] = lengths[i];
            voq_state.sum += lengths[i];
        }
    }
};

#endif // INPUT_PORT_H
