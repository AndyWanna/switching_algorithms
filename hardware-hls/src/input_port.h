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
    random_t lfsr_state;          // Local LFSR for randomness
    
public:
    InputPort() : port_id(0), availability(~avail_bitmap_t(0)), lfsr_state(0) {}
    
    void initialize(port_id_t id, random_t seed) {
        #pragma HLS INLINE
        port_id = id;
        lfsr_state = seed + id;  // Different seed per port
        availability = ~avail_bitmap_t(0);  // All slots initially available
        
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
        }
    }
    
    /*
     * Generate proposal using QPS
     * Returns: Proposal with sampled output port and availability
     */
    Proposal generateProposal() {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE
        
        Proposal prop;
        prop.input_id = port_id;
        prop.availability = availability;
        prop.valid = false;
        
        // Update LFSR for randomness
        lfsr_state = lfsr_next(lfsr_state);
        
        // Sample output port using QPS
        port_id_t sampled_output = QPSSampler::sample(voq_state, lfsr_state);
        
        if (sampled_output != INVALID_PORT && voq_state.lengths[sampled_output] > 0) {
            prop.voq_len = voq_state.lengths[sampled_output];
            prop.valid = true;
            return prop;
        }
        
        prop.valid = false;
        return prop;
    }
    
    /*
     * Process accept message from output port
     */
    void processAccept(const Accept& accept) {
        #pragma HLS INLINE
        if (accept.valid && accept.time_slot < T) {
            // Mark slot as unavailable
            availability &= ~(avail_bitmap_t(1) << accept.time_slot);
            voq_state.availability = availability;
        }
    }
    
    /*
     * Graduate senior slot and shift window
     */
    void graduateSlot(bool matched, port_id_t output_port) {
        #pragma HLS INLINE
        
        // If matched in graduating slot, remove packet
        if (matched && output_port < N) {
            removePacket(output_port);
        }
        
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
