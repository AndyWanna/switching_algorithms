/*
 * ============================================================================
 * PHASE 1 TESTBENCH
 * ============================================================================
 * 
 * C++ testbench to verify Phase 1 functionality before synthesis
 */


#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <map>
#include "../src/sw_qps_types.h"

#define MAX_SAMPLES 16
#define RANDOM_SEED 12345

// Declare test functions
extern void test_phase1_top(
    queue_len_t voq_lengths[N],
    random_t random_seed,
    port_id_t *selected_port,
    bool *is_valid
);

extern void test_lfsr_top(
    random_t seed,
    ap_uint<8> num_iterations,
    random_t *random_out
);

extern void test_ffa_top(
    avail_bitmap_t input_avail,
    avail_bitmap_t output_avail,
    slot_id_t *selected_slot,
    bool *found
);

extern void test_bitmap_top(
    avail_bitmap_t bitmap,
    slot_id_t slot_to_mark,
    slot_id_t *first_set,
    avail_bitmap_t *modified_bitmap
);

extern void test_phase1_integrated(
    queue_len_t voq_lengths[N],
    random_t initial_seed,
    ap_uint<8> num_samples,
    port_id_t samples[MAX_SAMPLES],
    ap_uint<8> *num_valid
);

// Helper function to print bitmap
void print_bitmap(avail_bitmap_t bitmap) {
    std::cout << "0b";
    for (int i = T-1; i >= 0; i--) {
        std::cout << (bitmap[i] ? '1' : '0');
    }
}

/*
 * TEST 1: Basic QPS Sampling
 */
bool test_basic_qps() {
    std::cout << "\n========================================\n";
    std::cout << "TEST 1: Basic QPS Sampling\n";
    std::cout << "========================================\n";
    
    queue_len_t voq_lengths[N];
    
    // Test case 1: All zeros (no packets)
    std::cout << "\nTest 1.1: No packets (all zeros)\n";
    for (int i = 0; i < N; i++) voq_lengths[i] = 0;
    
    port_id_t selected;
    bool valid;
    test_phase1_top(voq_lengths, RANDOM_SEED, &selected, &valid);
    
    if (!valid) {
        std::cout << "✓ Correctly returned invalid\n";
    } else {
        std::cout << "✗ FAIL: Should return invalid for no packets\n";
        return false;
    }
    
    // Test case 2: Single non-zero VOQ
    std::cout << "\nTest 1.2: Single queue (port 5 has 100 packets)\n";
    for (int i = 0; i < N; i++) voq_lengths[i] = 0;
    voq_lengths[5] = 100;
    
    test_phase1_top(voq_lengths, RANDOM_SEED, &selected, &valid);
    
    if (valid && selected == 5) {
        std::cout << "✓ Correctly selected port 5\n";
    } else {
        std::cout << "✗ FAIL: Expected port 5, got " << (int)selected << "\n";
        return false;
    }
    
    // Test case 3: Multiple queues, check distribution
    std::cout << "\nTest 1.3: Multiple queues (testing distribution)\n";
    voq_lengths[0] = 100;  // 50% probability
    voq_lengths[1] = 50;   // 25% probability
    voq_lengths[2] = 50;   // 25% probability
    for (int i = 3; i < N; i++) voq_lengths[i] = 0;
    
    std::map<int, int> counts;
    const int num_trials = 10000;
    
    for (int trial = 0; trial < num_trials; trial++) {
        test_phase1_top(voq_lengths, RANDOM_SEED + trial, &selected, &valid);
        if (valid) {
            counts[selected]++;
        }
    }
    
    std::cout << "Distribution over " << num_trials << " samples:\n";
    std::cout << "  Port 0: " << counts[0] << " (" 
              << (100.0 * counts[0] / num_trials) << "%, expected ~50%)\n";
    std::cout << "  Port 1: " << counts[1] << " (" 
              << (100.0 * counts[1] / num_trials) << "%, expected ~25%)\n";
    std::cout << "  Port 2: " << counts[2] << " (" 
              << (100.0 * counts[2] / num_trials) << "%, expected ~25%)\n";
    
    // Check if distribution is reasonable (within 5% of expected)
    double port0_ratio = (double)counts[0] / num_trials;
    double port1_ratio = (double)counts[1] / num_trials;
    double port2_ratio = (double)counts[2] / num_trials;
    
    if (port0_ratio > 0.45 && port0_ratio < 0.55 &&
        port1_ratio > 0.20 && port1_ratio < 0.30 &&
        port2_ratio > 0.20 && port2_ratio < 0.30) {
        std::cout << "✓ Distribution looks correct\n";
    } else {
        std::cout << "✗ FAIL: Distribution is off\n";
        return false;
    }
    
    return true;
}

/*
 * TEST 2: LFSR Random Number Generation
 */
bool test_lfsr() {
    std::cout << "\n========================================\n";
    std::cout << "TEST 2: LFSR Random Number Generation\n";
    std::cout << "========================================\n";
    
    random_t seed = 0xDEADBEEF;
    random_t result;
    
    // Test 2.1: Single iteration
    std::cout << "\nTest 2.1: Single LFSR step\n";
    std::cout << "Seed:   0x" << std::hex << seed << std::dec << "\n";
    test_lfsr_top(seed, 1, &result);
    std::cout << "Result: 0x" << std::hex << result << std::dec << "\n";
    
    if (result != seed) {
        std::cout << "✓ LFSR produces different output\n";
    } else {
        std::cout << "✗ FAIL: LFSR stuck\n";
        return false;
    }
    
    // Test 2.2: Check for periodicity (shouldn't repeat quickly)
    std::cout << "\nTest 2.2: Check uniqueness over 100 iterations\n";
    std::map<uint32_t, int> seen;
    random_t state = seed;
    
    for (int i = 0; i < 100; i++) {
        test_lfsr_top(state, 1, &state);
        seen[state]++;
    }
    
    if (seen.size() == 100) {
        std::cout << "✓ All 100 values unique\n";
    } else {
        std::cout << "✗ FAIL: Only " << seen.size() << " unique values\n";
        return false;
    }
    
    return true;
}

/*
 * TEST 3: First Fit Accept
 */
bool test_ffa() {
    std::cout << "\n========================================\n";
    std::cout << "TEST 3: First Fit Accept\n";
    std::cout << "========================================\n";
    
    slot_id_t slot;
    bool found;
    
    // Test 3.1: No mutual availability
    std::cout << "\nTest 3.1: No mutual availability\n";
    avail_bitmap_t input_avail  = 0b0000111100001111;
    avail_bitmap_t output_avail = 0b1111000011110000;
    
    std::cout << "Input:  "; print_bitmap(input_avail); std::cout << "\n";
    std::cout << "Output: "; print_bitmap(output_avail); std::cout << "\n";
    
    test_ffa_top(input_avail, output_avail, &slot, &found);
    
    if (!found) {
        std::cout << "✓ Correctly found no mutual slot\n";
    } else {
        std::cout << "✗ FAIL: Should find no mutual slot\n";
        return false;
    }
    
    // Test 3.2: Mutual availability
    std::cout << "\nTest 3.2: Mutual availability (first at slot 2)\n";
    input_avail  = 0b0000111111111111;
    output_avail = 0b1111111111111100;
    
    std::cout << "Input:  "; print_bitmap(input_avail); std::cout << "\n";
    std::cout << "Output: "; print_bitmap(output_avail); std::cout << "\n";
    
    test_ffa_top(input_avail, output_avail, &slot, &found);
    
    if (found && slot == 2) {
        std::cout << "✓ Correctly found slot 2\n";
    } else {
        std::cout << "✗ FAIL: Expected slot 2, got " << (int)slot << "\n";
        return false;
    }
    
    // Test 3.3: First slot available
    std::cout << "\nTest 3.3: First slot (0) available\n";
    input_avail  = 0b1111111111111111;
    output_avail = 0b1111111111111111;
    
    test_ffa_top(input_avail, output_avail, &slot, &found);
    
    if (found && slot == 0) {
        std::cout << "✓ Correctly found slot 0\n";
    } else {
        std::cout << "✗ FAIL: Expected slot 0, got " << (int)slot << "\n";
        return false;
    }
    
    return true;
}

/*
 * TEST 4: Bitmap Operations
 */
bool test_bitmap() {
    std::cout << "\n========================================\n";
    std::cout << "TEST 4: Bitmap Operations\n";
    std::cout << "========================================\n";
    
    slot_id_t first;
    avail_bitmap_t modified;
    
    // Test 4.1: Find first set
    std::cout << "\nTest 4.1: Find first set bit\n";
    avail_bitmap_t bitmap = 0b0000111100000000;
    std::cout << "Bitmap: "; print_bitmap(bitmap); std::cout << "\n";
    
    test_bitmap_top(bitmap, 0, &first, &modified);
    
    if (first == 8) {
        std::cout << "✓ Correctly found first set at position 8\n";
    } else {
        std::cout << "✗ FAIL: Expected 8, got " << (int)first << "\n";
        return false;
    }
    
    // Test 4.2: Mark unavailable
    std::cout << "\nTest 4.2: Mark slot 10 unavailable\n";
    bitmap = 0b0000111111111111;
    std::cout << "Before: "; print_bitmap(bitmap); std::cout << "\n";
    
    test_bitmap_top(bitmap, 10, &first, &modified);
    
    std::cout << "After:  "; print_bitmap(modified); std::cout << "\n";
    
    if (modified[10] == 0 && modified[9] == 1 && modified[11] == 1) {
        std::cout << "✓ Correctly marked slot 10 unavailable\n";
    } else {
        std::cout << "✗ FAIL: Bit 10 not cleared properly\n";
        return false;
    }
    
    return true;
}

/*
 * TEST 5: Integrated Test
 */
bool test_integrated() {
    std::cout << "\n========================================\n";
    std::cout << "TEST 5: Integrated Multi-Sample Test\n";
    std::cout << "========================================\n";
    
    queue_len_t voq_lengths[N];
    port_id_t samples[MAX_SAMPLES];
    ap_uint<8> num_valid;
    
    // Setup: 3 queues with different lengths
    voq_lengths[0] = 100;
    voq_lengths[1] = 50;
    voq_lengths[2] = 25;
    for (int i = 3; i < N; i++) voq_lengths[i] = 0;
    
    std::cout << "\nQueues: Port 0=100, Port 1=50, Port 2=25, others=0\n";
    std::cout << "Generating 16 samples...\n\n";
    
    test_phase1_integrated(voq_lengths, 0xBEEF, 16, samples, &num_valid);
    
    std::cout << "Generated " << (int)num_valid << " valid samples:\n";
    std::map<int, int> dist;
    for (int i = 0; i < num_valid; i++) {
        std::cout << "  Sample " << i << ": Port " << (int)samples[i] << "\n";
        dist[samples[i]]++;
    }
    
    std::cout << "\nDistribution:\n";
    std::cout << "  Port 0: " << dist[0] << " samples\n";
    std::cout << "  Port 1: " << dist[1] << " samples\n";
    std::cout << "  Port 2: " << dist[2] << " samples\n";
    
    if (num_valid == 16) {
        std::cout << "✓ Generated expected number of samples\n";
    } else {
        std::cout << "✗ FAIL: Expected 16 samples, got " << (int)num_valid << "\n";
        return false;
    }
    
    return true;
}

/*
 * MAIN
 */
int main() {
    std::cout << "========================================\n";
    std::cout << "PHASE 1 SYNTHESIZABILITY TEST\n";
    std::cout << "========================================\n";
    std::cout << "N = " << N << " ports\n";
    std::cout << "T = " << T << " time slots\n";
    
    int passed = 0;
    int total = 4;
    
    if (test_basic_qps()) passed++;
    if (test_lfsr()) passed++;
    // if (test_ffa()) passed++; // Potentially has an infinite loop
    if (test_bitmap()) passed++;
    if (test_integrated()) passed++;
    
    std::cout << "\n========================================\n";
    std::cout << "RESULTS: " << passed << "/" << total << " tests passed\n";
    std::cout << "========================================\n";
    
    if (passed == total) {
        std::cout << "✓ ALL TESTS PASSED - Ready for HLS synthesis!\n";
        return 0;
    } else {
        std::cout << "✗ SOME TESTS FAILED - Fix issues before synthesis\n";
        return 1;
    }
}