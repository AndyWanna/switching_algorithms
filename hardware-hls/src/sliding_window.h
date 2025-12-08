#ifndef SLIDING_WINDOW_H
#define SLIDING_WINDOW_H

#include "sw_qps_types.h"
#include "input_port.h"
#include "output_port.h"

/*
 * ============================================================================
 * SLIDING WINDOW MANAGER
 * ============================================================================
 * 
 * Coordinates SW-QPS algorithm:
 *   1. Manages N input ports and N output ports
 *   2. Runs iterations (propose-accept phases)
 *   3. Graduates senior matchings every cycle
 *   4. Maintains sliding window of T matchings
 */

struct MatchingResult {
    port_id_t matching[N];  // matching[output] = input (or INVALID_PORT)
    int matching_size;      // Number of matched pairs
    
    MatchingResult() : matching_size(0) {
        for (int i = 0; i < N; i++) {
            matching[i] = INVALID_PORT;
        }
    }
};

class SlidingWindowManager {
private:
    InputPort input_ports[N];
    OutputPort output_ports[N];
    int current_time_slot;
    slot_id_t current_frame_slot;  // Which slot in the frame (0 to T-1)

    // Statistics
    long total_matched_pairs;
    long total_iterations;

public:
    SlidingWindowManager() : current_time_slot(0),
                             current_frame_slot(0),
                             total_matched_pairs(0),
                             total_iterations(0) {}
    
    /*
     * Initialize all ports
     */
    void initialize(random_t seed = 12345) {
        #pragma HLS INLINE off

        current_time_slot = 0;
        current_frame_slot = 0;
        total_matched_pairs = 0;
        total_iterations = 0;

        // Initialize input ports with different seeds
        for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL factor=4
            input_ports[i].initialize(i, seed + i * 1000);
        }

        // Initialize output ports
        for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL factor=4
            output_ports[i].initialize(i);
        }
    }
    
    /*
     * Run one SW-QPS iteration (propose-accept)
     */
    void runIteration() {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE off
        
        // Arrays to collect proposals per output port
        Proposal proposals_per_output[N][N];  // [output][proposals]
        int num_proposals_per_output[N];
        
        #pragma HLS ARRAY_PARTITION variable=proposals_per_output dim=1 complete
        #pragma HLS ARRAY_PARTITION variable=num_proposals_per_output complete
        
        // Initialize
        for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL
            num_proposals_per_output[i] = 0;
        }
        
        // Phase 1: Generate proposals from all input ports
        PROPOSE_PHASE: for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL factor=4
            
            Proposal prop = input_ports[i].generateProposal();
            
            if (prop.valid) {
                // Now we have the output port directly from the proposal!
                port_id_t target_output = prop.output_id;
                
                if (target_output != INVALID_PORT && target_output < N) {
                    int idx = num_proposals_per_output[target_output];
                    if (idx < N) {
                        proposals_per_output[target_output][idx] = prop;
                        num_proposals_per_output[target_output]++;
                    }
                }
            }
        }
        
        // Phase 2: Each output port processes its proposals
        ACCEPT_PHASE: for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL factor=4
            
            Accept accepts[N];
            int num_accepts = 0;
            
            output_ports[i].processProposals(
                proposals_per_output[i],
                num_proposals_per_output[i],
                accepts,
                num_accepts,
                current_frame_slot  // Pass current slot for backfilling logic
            );
            
            // Route accepts back to input ports
            for (int j = 0; j < num_accepts; j++) {
                #pragma HLS LOOP_TRIPCOUNT min=0 max=1
                if (accepts[j].valid && accepts[j].input_id < N) {
                    // Send accept only to the specific input port that was accepted
                    input_ports[accepts[j].input_id].processAccept(accepts[j]);
                }
            }
        }

        total_iterations++;

        // Increment frame slot (wraps around at T)
        current_frame_slot++;
        if (current_frame_slot >= T) {
            current_frame_slot = 0;
        }
    }
    
    /*
     * Graduate current matching and shift window
     * Returns the graduated matching
     */
    MatchingResult graduateMatching() {
        #pragma HLS INLINE off
        
        MatchingResult result;
        result.matching_size = 0;
        
        // Collect senior matches from all output ports
        for (int output = 0; output < N; output++) {
            #pragma HLS UNROLL factor=4
            
            port_id_t input = output_ports[output].graduateSlot();
            result.matching[output] = input;
            
            if (input != INVALID_PORT) {
                result.matching_size++;
                total_matched_pairs++;
                
                // Notify input port about the match
                input_ports[input].graduateSlot(true, output);
            }
        }
        
        // Update non-matched input ports
        for (int input = 0; input < N; input++) {
            #pragma HLS UNROLL factor=4
            
            bool matched = false;
            for (int output = 0; output < N; output++) {
                if (result.matching[output] == input) {
                    matched = true;
                    break;
                }
            }
            
            if (!matched) {
                input_ports[input].graduateSlot(false, INVALID_PORT);
            }
        }
        
        current_time_slot++;
        return result;
    }
    
    /*
     * Add packets to VOQs (for traffic generation)
     */
    void addPacket(port_id_t input, port_id_t output) {
        #pragma HLS INLINE
        if (input < N && output < N) {
            input_ports[input].addPacket(output);
        }
    }
    
    /*
     * Load traffic pattern (for testing)
     */
    void loadTrafficMatrix(queue_len_t traffic_matrix[N][N]) {
        #pragma HLS INLINE off
        for (int i = 0; i < N; i++) {
            #pragma HLS UNROLL factor=4
            input_ports[i].loadTraffic(traffic_matrix[i]);
        }
    }
    
    /*
     * Get statistics
     */
    void getStatistics(
        long& matched_pairs,
        long& iterations,
        double& avg_matching_size
    ) {
        #pragma HLS INLINE
        matched_pairs = total_matched_pairs;
        iterations = total_iterations;
        avg_matching_size = (iterations > 0) ? 
            (double)matched_pairs / iterations : 0.0;
    }
    
    /*
     * Get VOQ occupancy (for monitoring)
     */
    void getVOQOccupancy(queue_len_t occupancy[N][N]) {
        #pragma HLS INLINE off
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                #pragma HLS UNROLL factor=4
                occupancy[i][j] = input_ports[i].getVOQLength(j);
            }
        }
    }
    
    /*
     * Check if system is stable (VOQs not growing unbounded)
     */
    bool isStable(queue_len_t threshold = MAX_VOQ_LEN / 2) {
        #pragma HLS INLINE off
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (input_ports[i].getVOQLength(j) > threshold) {
                    return false;
                }
            }
        }
        return true;
    }
};

#endif // SLIDING_WINDOW_H
