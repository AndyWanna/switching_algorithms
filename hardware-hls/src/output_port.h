#ifndef OUTPUT_PORT_H
#define OUTPUT_PORT_H

#include "sw_qps_types.h"
#include "utils.h"
#include <algorithm>

/*
 * ============================================================================
 * OUTPUT PORT MODULE
 * ============================================================================
 * 
 * Manages calendar and accepts/rejects proposals
 * Each output port:
 *   1. Maintains a calendar of T time slots
 *   2. Receives proposals from input ports
 *   3. Sorts proposals by VOQ length
 *   4. Accepts using First-Fit Accept (FFA)
 *   5. Sends accept messages back
 */

class OutputPort {
private:
    port_id_t port_id;
    Calendar calendar;  // Schedule for T time slots
    
public:
    OutputPort() : port_id(0) {}
    
    void initialize(port_id_t id) {
        #pragma HLS INLINE
        port_id = id;
        
        // Initialize calendar
        for (int i = 0; i < T; i++) {
            #pragma HLS UNROLL
            calendar.schedule[i] = INVALID_PORT;
        }
        calendar.availability = ~avail_bitmap_t(0);  // All slots available
    }
    
    /*
     * Process multiple proposals and select best ones
     * Uses First-Fit Accept (FFA) strategy
     * 
     * Input: Array of proposals and count
     * Output: Array of accepts
     */
    void processProposals(
        Proposal proposals[N],  // Max N proposals (one per input)
        int num_proposals,
        Accept accepts[N],      // Output accepts
        int& num_accepts
    ) {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE off

        num_accepts = 0;

        if (num_proposals == 0) {
            return;
        }
        
        // Sort proposals by VOQ length (descending)
        // For HLS, use a simple sorting network for small arrays
        Proposal sorted_proposals[KNOCKOUT_THRESH];
        #pragma HLS ARRAY_PARTITION variable=sorted_proposals complete
        
        // Take top KNOCKOUT_THRESH proposals
        int num_to_process = (num_proposals > KNOCKOUT_THRESH) ? KNOCKOUT_THRESH : num_proposals;
        
        // Simple selection sort for top-K (works well for small K)
        for (int i = 0; i < num_to_process; i++) {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=3
            int max_idx = i;
            queue_len_t max_len = proposals[i].voq_len;
            
            for (int j = i + 1; j < num_proposals && j < N; j++) {
                #pragma HLS LOOP_TRIPCOUNT min=0 max=64
                if (proposals[j].voq_len > max_len) {
                    max_idx = j;
                    max_len = proposals[j].voq_len;
                }
            }
            
            // Swap
            if (max_idx != i) {
                Proposal temp = proposals[i];
                proposals[i] = proposals[max_idx];
                proposals[max_idx] = temp;
            }
            
            sorted_proposals[i] = proposals[i];
        }
        
        // Try to accept proposals using FFA
        for (int i = 0; i < num_to_process; i++) {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=3
            
            if (!sorted_proposals[i].valid) continue;
            
            // Find first mutual available slot
            slot_id_t slot = first_fit_accept(
                sorted_proposals[i].availability,
                calendar.availability
            );
            
            if (slot != INVALID_PORT) {
                // Accept the proposal
                calendar.schedule[slot] = sorted_proposals[i].input_id;
                calendar.availability &= ~(avail_bitmap_t(1) << slot);

                // Create accept message
                Accept acc;
                acc.output_id = port_id;
                acc.input_id = sorted_proposals[i].input_id;
                acc.time_slot = slot;
                acc.valid = true;

                accepts[num_accepts++] = acc;

                // In standard SW-QPS, accept only one per iteration
                break;
            }
        }
    }
    
    /*
     * Graduate senior slot and shift window
     * Returns the input port that was matched (or INVALID_PORT)
     */
    port_id_t graduateSlot() {
        #pragma HLS INLINE
        
        // Get matched input for slot 0
        port_id_t matched_input = calendar.schedule[0];
        
        // Shift schedule left
        for (int i = 0; i < T-1; i++) {
            #pragma HLS UNROLL
            calendar.schedule[i] = calendar.schedule[i+1];
        }
        calendar.schedule[T-1] = INVALID_PORT;
        
        // Shift availability and add new slot
        calendar.availability = (calendar.availability >> 1) | (avail_bitmap_t(1) << (T-1));
        
        return matched_input;
    }
    
    /*
     * Get current matching for senior slot
     */
    port_id_t getSeniorMatch() const {
        #pragma HLS INLINE
        return calendar.schedule[0];
    }
    
    /*
     * Get availability bitmap
     */
    avail_bitmap_t getAvailability() const {
        #pragma HLS INLINE
        return calendar.availability;
    }
    
    /*
     * Get schedule (for debugging)
     */
    void getSchedule(port_id_t schedule[T]) const {
        #pragma HLS INLINE off
        for (int i = 0; i < T; i++) {
            #pragma HLS UNROLL
            schedule[i] = calendar.schedule[i];
        }
    }
    
    /*
     * Check if a specific slot is available
     */
    bool isSlotAvailable(slot_id_t slot) const {
        #pragma HLS INLINE
        return (slot < T) && ((calendar.availability & (avail_bitmap_t(1) << slot)) != 0);
    }
    
    /*
     * Manually set a match (for testing)
     */
    void setMatch(slot_id_t slot, port_id_t input) {
        #pragma HLS INLINE
        if (slot < T && isSlotAvailable(slot)) {
            calendar.schedule[slot] = input;
            calendar.availability &= ~(avail_bitmap_t(1) << slot);
        }
    }
};

#endif // OUTPUT_PORT_H
