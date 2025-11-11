/*
 * ============================================================================
 * PHASE 1 SYNTHESIZABILITY TEST
 * ============================================================================
 * 
 * Tests basic QPS sampler synthesis without full SW-QPS complexity
 * 
 * This module:
 *   1. Takes VOQ lengths as input
 *   2. Runs QPS sampling
 *   3. Outputs selected port
 * 
 * Goal: Verify HLS can synthesize the core QPS logic
 */

#include "sw_qps_types.h"
#include "qps_sampler.cpp"
#include "utils.h"
/*
 * SIMPLE QPS TEST TOP
 * 
 * Inputs:
 *   voq_lengths[N]: Queue lengths for one input port
 *   random_seed: Random number for sampling
 * 
 * Outputs:
 *   selected_port: Which output port was sampled
 *   is_valid: Whether selection is valid (false if no packets)
 * 
 * Pseudocode:
 *   // Compute sum of VOQ lengths
 *   sum = 0
 *   FOR i = 0 to N-1:
 *     sum += voq_lengths[i]
 *   
 *   // Sample using QPS
 *   selected_port = qps_sample(voq_lengths, sum, random_seed)
 *   
 *   // Check validity
 *   IF sum == 0:
 *     is_valid = false
 *   ELSE:
 *     is_valid = true
 */
void test_phase1_top(
    // Inputs
    queue_len_t voq_lengths[N],
    random_t random_seed,
    
    // Outputs
    port_id_t *selected_port,
    bool *is_valid
) {
    // HLS Interface directives
    #pragma HLS INTERFACE mode=ap_ctrl_hs port=return
    #pragma HLS INTERFACE mode=ap_none port=voq_lengths
    #pragma HLS INTERFACE mode=ap_none port=random_seed
    #pragma HLS INTERFACE mode=ap_none port=selected_port
    #pragma HLS INTERFACE mode=ap_none port=is_valid
    
    // Partition array for parallelism
    #pragma HLS ARRAY_PARTITION variable=voq_lengths complete
    
    // Pipeline the entire function
    #pragma HLS PIPELINE II=1
    
    // Create VOQ state structure
    VOQState voq_state;
    
    // Copy lengths
    COPY_LENGTHS: for (int i = 0; i < N; i++) {
        #pragma HLS UNROLL
        voq_state.lengths[i] = voq_lengths[i];
    }
    
    // Compute sum
    voq_state.sum = compute_voq_sum(voq_state.lengths);
    
    // Sample using QPS
    port_id_t result = QPSSampler::sample(voq_state, random_seed);
    
    // Set outputs
    *selected_port = result;
    *is_valid = (result != INVALID_PORT);
}

/*
 * LFSR TEST TOP
 * 
 * Tests LFSR random number generation
 * 
 * Inputs:
 *   seed: Initial LFSR state
 *   num_iterations: How many random numbers to generate
 * 
 * Outputs:
 *   random_out: Final random number after num_iterations
 * 
 * Pseudocode:
 *   state = seed
 *   FOR i = 0 to num_iterations-1:
 *     state = lfsr_next(state)
 *   random_out = state
 */
void test_lfsr_top(
    random_t seed,
    ap_uint<8> num_iterations,
    random_t *random_out
) {
    #pragma HLS INTERFACE mode=ap_ctrl_hs port=return
    #pragma HLS INTERFACE mode=ap_none port=seed
    #pragma HLS INTERFACE mode=ap_none port=num_iterations
    #pragma HLS INTERFACE mode=ap_none port=random_out
    
    #pragma HLS PIPELINE II=1
    
    random_t state = seed;
    
    LFSR_LOOP: for (int i = 0; i < num_iterations; i++) {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=255
        state = lfsr_next(state);
    }
    
    *random_out = state;
}

/*
 * FIRST FIT ACCEPT TEST TOP
 * 
 * Tests the first-fit accept logic
 * 
 * Inputs:
 *   input_avail: 16-bit availability bitmap for input
 *   output_avail: 16-bit availability bitmap for output
 * 
 * Outputs:
 *   selected_slot: First mutual available slot
 *   found: Whether a slot was found
 * 
 * Pseudocode:
 *   slot = first_fit_accept(input_avail, output_avail)
 *   
 *   IF slot == INVALID_PORT:
 *     found = false
 *     selected_slot = 0
 *   ELSE:
 *     found = true
 *     selected_slot = slot
 */
void test_ffa_top(
    avail_bitmap_t input_avail,
    avail_bitmap_t output_avail,
    slot_id_t *selected_slot,
    bool *found
) {
    #pragma HLS INTERFACE mode=ap_ctrl_hs port=return
    #pragma HLS INTERFACE mode=ap_none port=input_avail
    #pragma HLS INTERFACE mode=ap_none port=output_avail
    #pragma HLS INTERFACE mode=ap_none port=selected_slot
    #pragma HLS INTERFACE mode=ap_none port=found
    
    #pragma HLS PIPELINE II=1
    
    slot_id_t result = first_fit_accept(input_avail, output_avail);
    
    if (result == INVALID_PORT) {
        *found = false;
        *selected_slot = 0;
    } else {
        *found = true;
        *selected_slot = result;
    }
}

/*
 * BITMAP OPERATIONS TEST TOP
 * 
 * Tests various bitmap operations
 * 
 * Inputs:
 *   bitmap: 16-bit bitmap
 *   slot_to_mark: Slot to mark as unavailable
 * 
 * Outputs:
 *   first_set: Position of first set bit
 *   modified_bitmap: Bitmap after marking slot unavailable
 */
void test_bitmap_top(
    avail_bitmap_t bitmap,
    slot_id_t slot_to_mark,
    slot_id_t *first_set,
    avail_bitmap_t *modified_bitmap
) {
    #pragma HLS INTERFACE mode=ap_ctrl_hs port=return
    #pragma HLS INTERFACE mode=ap_none port=bitmap
    #pragma HLS INTERFACE mode=ap_none port=slot_to_mark
    #pragma HLS INTERFACE mode=ap_none port=first_set
    #pragma HLS INTERFACE mode=ap_none port=modified_bitmap
    
    #pragma HLS PIPELINE II=1
    
    // Find first set bit
    *first_set = find_first_set(bitmap);
    
    // Mark slot unavailable
    avail_bitmap_t temp = bitmap;
    if (slot_to_mark < T) {
        mark_unavailable(temp, slot_to_mark);
    }
    *modified_bitmap = temp;
}

/*
 * INTEGRATED PHASE 1 TEST
 * 
 * Tests multiple QPS samples in sequence
 * Demonstrates iterative usage
 * 
 * Inputs:
 *   voq_lengths[N]: Queue lengths
 *   initial_seed: Starting random seed
 *   num_samples: How many samples to generate
 * 
 * Outputs:
 *   samples[MAX_SAMPLES]: Array of sampled ports
 *   num_valid: How many valid samples were generated
 * 
 * Pseudocode:
 *   seed = initial_seed
 *   count = 0
 *   
 *   FOR i = 0 to num_samples-1:
 *     seed = lfsr_next(seed)
 *     port = qps_sample(voq_lengths, seed)
 *     
 *     IF port != INVALID_PORT:
 *       samples[count] = port
 *       count++
 *   
 *   num_valid = count
 */
#define MAX_SAMPLES 16

void test_phase1_integrated(
    queue_len_t voq_lengths[N],
    random_t initial_seed,
    ap_uint<8> num_samples,
    port_id_t samples[MAX_SAMPLES],
    ap_uint<8> *num_valid
) {
    #pragma HLS INTERFACE mode=ap_ctrl_hs port=return
    #pragma HLS INTERFACE mode=ap_none port=voq_lengths
    #pragma HLS INTERFACE mode=ap_none port=initial_seed
    #pragma HLS INTERFACE mode=ap_none port=num_samples
    #pragma HLS INTERFACE mode=ap_none port=samples
    #pragma HLS INTERFACE mode=ap_none port=num_valid
    
    #pragma HLS ARRAY_PARTITION variable=voq_lengths complete
    #pragma HLS ARRAY_PARTITION variable=samples complete
    
    // Create VOQ state
    VOQState voq_state;
    
    // Copy lengths and compute sum
    INIT: for (int i = 0; i < N; i++) {
        #pragma HLS UNROLL
        voq_state.lengths[i] = voq_lengths[i];
    }
    voq_state.sum = compute_voq_sum(voq_state.lengths);
    
    // Generate samples
    random_t seed = initial_seed;
    ap_uint<8> count = 0;
    
    SAMPLE_LOOP: for (ap_uint<8> i = 0; i < num_samples && i < MAX_SAMPLES; i++) {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=16
        #pragma HLS PIPELINE II=1
        
        // Generate next random number
        seed = lfsr_next(seed);
        
        // Sample port
        port_id_t port = QPSSampler::sample(voq_state, seed);
        
        // Store if valid
        if (port != INVALID_PORT && count < MAX_SAMPLES) {
            samples[count] = port;
            count++;
        }
    }
    
    *num_valid = count;
}

// void top(    // Inputs
//     queue_len_t voq_lengths[N],
//     random_t random_seed,
    
//     // Outputs
//     port_id_t *selected_port,
//     bool *is_valid) {
//     // Top function placeholder

//     test_phase1_top(voq_lengths, random_seed, selected_port, is_valid);

// }