#ifndef QPS_SAMPLER_H
#define QPS_SAMPLER_H

#include "sw_qps_types.h"
#include "utils.h"

/*
 * ============================================================================
 * QUEUE-PROPORTIONAL SAMPLING (QPS)
 * ============================================================================
 * 
 * Goal: Sample output port j with probability proportional to VOQ length
 * 
 * Method: Simple sampling (can optimize with alias method later)
 * 
 * Pseudocode:
 *   1. Compute sum of all VOQ lengths
 *   2. Generate random number in range [0, sum)
 *   3. Walk through VOQs, accumulating lengths until we pass random number
 *   4. Return that VOQ's output port
 * 
 * Example:
 *   VOQ lengths: [100, 50, 25, 0, 0, ..., 0]
 *   Sum: 175
 *   Random: 123
 *   
 *   Cumulative: 0 -> 100 (skip) -> 150 (123 < 150, select port 1!)
 * 
 * Time Complexity: O(N) worst case, but can be O(1) with alias method
 */

class QPSSampler {
public:
    /*
     * Sample an output port using QPS
     * 
     * Inputs:
     *   voq_state: Current VOQ state (lengths and sum)
     *   random_num: Random number for sampling
     * 
     * Output: Selected output port ID, or INVALID_PORT if no packets
     * 
     * Pseudocode:
     *   IF voq_state.sum == 0:
     *     RETURN INVALID_PORT  // No packets to send
     *   
     *   target = random_num % voq_state.sum
     *   cumsum = 0
     *   
     *   FOR i = 0 to N-1:
     *     cumsum += voq_state.lengths[i]
     *     IF target < cumsum:
     *       RETURN i
     *   
     *   RETURN 0  // Shouldn't reach here
     */
    static port_id_t sample(VOQState &voq_state, random_t random_num) {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE
        
        // Check if there are any packets to send
        if (voq_state.sum == 0) {
            return INVALID_PORT;
        }
        
        // Generate target value
        queue_len_t target = random_num % voq_state.sum;
        
        // Accumulate until we reach target
        queue_len_t cumsum = 0;
        for (port_id_t i = 0; i < N; i++) {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=64
            cumsum += voq_state.lengths[i];
            if (target < cumsum) {
                return i;
            }
        }
        
        // Should never reach here
        return 0;
    }
    
    /*
     * OPTIMIZED VERSION: Alias Method (O(1) sampling)
     * 
     * This is more complex but gives true O(1) sampling.
     * Implement this after getting basic version working.
     * 
     * Requires pre-computing two tables:
     *   - alias[i]: Alternative port for bucket i
     *   - prob[i]: Probability threshold for bucket i
     * 
     * Pseudocode:
     *   bucket = random_num % N
     *   threshold = (random_num / N) % voq_state.sum
     *   
     *   IF threshold < prob[bucket]:
     *     RETURN bucket
     *   ELSE:
     *     RETURN alias[bucket]
     */
    static port_id_t sample_optimized(
        VOQState &voq_state,
        random_t random_num,
        port_id_t alias_table[N],
        queue_len_t prob_table[N]
    ) {
        #pragma HLS INLINE
        
        if (voq_state.sum == 0) {
            return INVALID_PORT;
        }
        
        port_id_t bucket = random_num % N;
        queue_len_t threshold = (random_num >> 6) % voq_state.sum;
        
        if (threshold < prob_table[bucket]) {
            return bucket;
        } else {
            return alias_table[bucket];
        }
    }
};

#endif // QPS_SAMPLER_H