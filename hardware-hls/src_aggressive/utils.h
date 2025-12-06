#ifndef UTILS_H
#define UTILS_H

#include "sw_qps_types.h"

// ============================================================================
// RANDOM NUMBER GENERATION
// ============================================================================

/*
 * LFSR (Linear Feedback Shift Register) for random number generation
 * 
 * Pseudocode:
 *   bit = state[31] XOR state[21] XOR state[1] XOR state[0]
 *   state = (state << 1) | bit
 *   return state
 */
inline random_t lfsr_next(random_t state) {
    #pragma HLS INLINE
    
    // Tap positions for maximal-length LFSR
    bool feedback = state[31] ^ state[21] ^ state[1] ^ state[0];
    return (state << 1) | feedback;
}

// ============================================================================
// BITMAP OPERATIONS
// ============================================================================

/*
 * Find first set bit (first available slot)
 * 
 * Input: 16-bit bitmap where 1 = available
 * Output: Position of first 1 bit (0-15), or INVALID_PORT if all zeros
 * 
 * Pseudocode:
 *   FOR i = 0 to T-1:
 *     IF bitmap[i] == 1:
 *       RETURN i
 *   RETURN INVALID_PORT
 */
inline slot_id_t find_first_set(avail_bitmap_t bitmap) {
    #pragma HLS INLINE
    
    // Priority encoder - synthesizes to efficient hardware
    for (slot_id_t i = 0; i < T; i++) {
        #pragma HLS UNROLL
        if (bitmap[i] == 1) {
            return i;
        }
    }
    return INVALID_PORT;
}

/*
 * First Fit Accept: Find earliest mutual availability
 * 
 * Inputs:
 *   input_avail: Input port's availability bitmap
 *   output_avail: Output port's availability bitmap
 * 
 * Output: First slot where both are available, or INVALID_PORT
 * 
 * Pseudocode:
 *   mutual = input_avail AND output_avail  // Bitwise AND
 *   RETURN find_first_set(mutual)
 */
inline slot_id_t first_fit_accept(
    avail_bitmap_t input_avail,
    avail_bitmap_t output_avail
) {
    #pragma HLS INLINE
    
    avail_bitmap_t mutual = input_avail & output_avail;
    return find_first_set(mutual);
}

/*
 * Mark slot as unavailable in bitmap
 * 
 * Pseudocode:
 *   bitmap[slot] = 0
 */
inline void mark_unavailable(avail_bitmap_t &bitmap, slot_id_t slot) {
    #pragma HLS INLINE
    bitmap[slot] = 0;
}

// ============================================================================
// VOQ OPERATIONS
// ============================================================================

/*
 * Update VOQ sum (total packets across all VOQs)
 * 
 * Pseudocode:
 *   sum = 0
 *   FOR i = 0 to N-1:
 *     sum += voq_lengths[i]
 *   RETURN sum
 */
inline queue_len_t compute_voq_sum(queue_len_t voq_lengths[N]) {
    #pragma HLS INLINE
    
    queue_len_t sum = 0;
    for (int i = 0; i < N; i++) {
        #pragma HLS UNROLL factor=4
        sum += voq_lengths[i];
    }
    return sum;
}

#endif // UTILS_H