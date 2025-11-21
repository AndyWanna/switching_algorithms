/*
 * ============================================================================
 * SW-QPS PURE C++ TESTBENCH
 * ============================================================================
 * 
 * Tests SW-QPS algorithm without HLS constraints
 * Verifies correctness before synthesis
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <cassert>
#include <cstring>
#include "../src/sw_qps_top.h"

using namespace std;

// Traffic pattern generators
class TrafficGenerator {
protected:
    mt19937 rng;
    double load;
    
public:
    TrafficGenerator(double l, unsigned seed = 42) : rng(seed), load(l) {}
    virtual ~TrafficGenerator() {}
    virtual bool shouldGeneratePacket() = 0;
    virtual int selectOutputPort(int input_port) = 0;
    virtual string getName() = 0;
    double getLoad() const { return load; }
};

class UniformTraffic : public TrafficGenerator {
public:
    UniformTraffic(double l, unsigned seed = 42) : TrafficGenerator(l, seed) {}
    
    bool shouldGeneratePacket() override {
        uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(rng) < load;
    }
    
    int selectOutputPort(int input_port) override {
        uniform_int_distribution<int> dist(0, N-1);
        return dist(rng);
    }
    
    string getName() override { return "Uniform"; }
};

class DiagonalTraffic : public TrafficGenerator {
public:
    DiagonalTraffic(double l, unsigned seed = 42) : TrafficGenerator(l, seed) {}
    
    bool shouldGeneratePacket() override {
        uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(rng) < load;
    }
    
    int selectOutputPort(int input_port) override {
        uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) < 2.0/3.0) {
            return input_port;  // Diagonal
        } else {
            return (input_port + 1) % N;  // Next neighbor
        }
    }
    
    string getName() override { return "Diagonal"; }
};

class QuasiDiagonalTraffic : public TrafficGenerator {
public:
    QuasiDiagonalTraffic(double l, unsigned seed = 42) : TrafficGenerator(l, seed) {}
    
    bool shouldGeneratePacket() override {
        uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(rng) < load;
    }
    
    int selectOutputPort(int input_port) override {
        uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) < 0.5) {
            return input_port;  // Diagonal
        } else {
            // Random non-diagonal
            uniform_int_distribution<int> port_dist(0, N-2);
            int port = port_dist(rng);
            if (port >= input_port) port++;
            return port;
        }
    }
    
    string getName() override { return "Quasi-Diagonal"; }
};

// Test functions
void test_basic_matching() {
    cout << "\n=== Test 1: Basic Matching ===" << endl;
    
    PacketArrival arrivals[N];
    port_id_t matching[N];
    ap_uint<8> matching_size;
    bool stable = false;
    
    // Reset system
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, true);
    
    // Add some packets
    for (int i = 0; i < N; i++) {
        arrivals[i].valid = false;
    }
    
    // Add diagonal traffic
    for (int i = 0; i < 10; i++) {
        arrivals[i].input_port = i;
        arrivals[i].output_port = i;
        arrivals[i].valid = true;
    }
    
    // Process arrivals
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, false);
    
    // Clear arrivals
    for (int i = 0; i < N; i++) {
        arrivals[i].valid = false;
    }
    
    // Run iterations
    for (int iter = 0; iter < T; iter++) {
        sw_qps_top(arrivals, true, false, matching, matching_size, stable, false);
    }
    
    // Graduate matching
    sw_qps_top(arrivals, false, true, matching, matching_size, stable, false);
    
    cout << "Matching size: " << (int)matching_size << endl;
    cout << "First 10 matches:" << endl;
    for (int i = 0; i < 10; i++) {
        if (matching[i] != INVALID_PORT) {
            cout << "  Output " << i << " <- Input " << (int)matching[i] << endl;
        }
    }
    
    // Verify diagonal matches
    int diagonal_matches = 0;
    for (int i = 0; i < 10; i++) {
        if (matching[i] == i) diagonal_matches++;
    }
    
    cout << "Diagonal matches: " << diagonal_matches << "/10" << endl;
    assert(diagonal_matches >= 8);  // Should match most diagonal pairs
    cout << "✓ Basic matching test passed" << endl;
}

void test_no_conflicts() {
    cout << "\n=== Test 2: No Conflicts ===" << endl;
    
    PacketArrival arrivals[N];
    port_id_t matching[N];
    ap_uint<8> matching_size;
    bool stable = false;
    
    // Reset
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, true);
    
    // Add full mesh traffic
    for (int i = 0; i < N; i++) {
        arrivals[i].input_port = i;
        arrivals[i].output_port = (i + 5) % N;  // Offset pattern
        arrivals[i].valid = true;
    }
    
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, false);
    
    // Clear arrivals and run iterations
    for (int i = 0; i < N; i++) arrivals[i].valid = false;
    
    for (int iter = 0; iter < T; iter++) {
        sw_qps_top(arrivals, true, false, matching, matching_size, stable, false);
    }
    
    // Graduate
    sw_qps_top(arrivals, false, true, matching, matching_size, stable, false);
    
    // Check for conflicts
    bool input_used[N] = {false};
    bool output_used[N] = {false};
    int conflicts = 0;
    
    for (int out = 0; out < N; out++) {
        if (matching[out] != INVALID_PORT) {
            int in = matching[out];
            if (input_used[in]) {
                cout << "ERROR: Input " << in << " used multiple times!" << endl;
                conflicts++;
            }
            if (output_used[out]) {
                cout << "ERROR: Output " << out << " used multiple times!" << endl;
                conflicts++;
            }
            input_used[in] = true;
            output_used[out] = true;
        }
    }
    
    cout << "Matching size: " << (int)matching_size << endl;
    cout << "Conflicts detected: " << conflicts << endl;
    assert(conflicts == 0);
    cout << "✓ No conflicts test passed" << endl;
}

void test_traffic_pattern(TrafficGenerator* traffic_gen, int num_cycles = 1000) {
    cout << "\n=== Test: " << traffic_gen->getName() 
         << " Traffic (Load=" << traffic_gen->getLoad() << ") ===" << endl;
    
    PacketArrival arrivals[N];
    port_id_t matching[N];
    ap_uint<8> matching_size;
    bool stable = false;
    
    // Reset
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, true);
    
    // Statistics
    long total_arrivals = 0;
    long total_departures = 0;
    vector<int> voq_lengths(N * N, 0);
    
    // Run simulation
    for (int cycle = 0; cycle < num_cycles; cycle++) {
        // Generate arrivals
        for (int i = 0; i < N; i++) {
            if (traffic_gen->shouldGeneratePacket()) {
                int output = traffic_gen->selectOutputPort(i);
                arrivals[i].input_port = i;
                arrivals[i].output_port = output;
                arrivals[i].valid = true;
                total_arrivals++;
                voq_lengths[i * N + output]++;
            } else {
                arrivals[i].valid = false;
            }
        }
        
        // Add arrivals
        sw_qps_top(arrivals, false, false, matching, matching_size, stable, false);
        
        // Clear arrivals
        for (int i = 0; i < N; i++) arrivals[i].valid = false;
        
        // Run one iteration per cycle (can adjust)
        sw_qps_top(arrivals, true, false, matching, matching_size, stable, false);
        
        // Graduate every cycle
        sw_qps_top(arrivals, false, true, matching, matching_size, stable, false);
        
        // Count departures
        for (int out = 0; out < N; out++) {
            if (matching[out] != INVALID_PORT) {
                total_departures++;
                int in = matching[out];
                if (voq_lengths[in * N + out] > 0) {
                    voq_lengths[in * N + out]--;
                }
            }
        }
        
        // Check stability periodically
        if (cycle % 100 == 0 && !stable) {
            cout << "  Warning: System unstable at cycle " << cycle << endl;
        }
    }
    
    // Calculate metrics
    double throughput = (double)total_departures / num_cycles / N;
    double normalized_throughput = throughput / traffic_gen->getLoad();
    
    // Find max VOQ length
    int max_voq = *max_element(voq_lengths.begin(), voq_lengths.end());
    double avg_voq = accumulate(voq_lengths.begin(), voq_lengths.end(), 0.0) / (N * N);
    
    cout << "Results:" << endl;
    cout << "  Total arrivals: " << total_arrivals << endl;
    cout << "  Total departures: " << total_departures << endl;
    cout << "  Throughput: " << fixed << setprecision(4) << throughput << endl;
    cout << "  Normalized throughput: " << normalized_throughput << endl;
    cout << "  Max VOQ length: " << max_voq << endl;
    cout << "  Avg VOQ length: " << avg_voq << endl;
    cout << "  System stable: " << (stable ? "Yes" : "No") << endl;
    
    // Basic sanity checks
    assert(normalized_throughput > 0.8);  // Should achieve > 80% throughput
    assert(normalized_throughput <= 1.01);  // Can't exceed 100% (with rounding)
    
    cout << "✓ Traffic test passed" << endl;
}

void test_single_cycle_interface() {
    cout << "\n=== Test 3: Single Cycle Interface ===" << endl;
    
    queue_len_t voq_state[N][N];
    port_id_t matching[N];
    ap_uint<8> matching_size;
    
    // Reset
    sw_qps_single_cycle(voq_state, 1, matching, matching_size, true);
    
    // Setup diagonal VOQs
    memset(voq_state, 0, sizeof(voq_state));
    for (int i = 0; i < 16; i++) {
        voq_state[i][i] = 10;  // 10 packets on diagonal
    }
    
    // Run with different iteration counts
    for (int iters = 1; iters <= 8; iters *= 2) {
        sw_qps_single_cycle(voq_state, iters, matching, matching_size, false);
        cout << "Iterations: " << iters << ", Matching size: " << (int)matching_size << endl;
    }
    
    assert(matching_size > 0);
    cout << "✓ Single cycle interface test passed" << endl;
}

void test_sliding_window() {
    cout << "\n=== Test 4: Sliding Window Behavior ===" << endl;
    
    PacketArrival arrivals[N];
    port_id_t matching[N];
    ap_uint<8> matching_size;
    bool stable = false;
    
    // Reset
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, true);
    
    // Add burst of packets
    cout << "Adding burst at time 0..." << endl;
    for (int i = 0; i < 8; i++) {
        arrivals[i].input_port = i;
        arrivals[i].output_port = i;
        arrivals[i].valid = true;
    }
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, false);
    
    // Clear arrivals
    for (int i = 0; i < N; i++) arrivals[i].valid = false;
    
    // Track matching sizes over time
    vector<int> sizes;
    
    // Run for 2*T cycles to see window effect
    for (int cycle = 0; cycle < 2*T; cycle++) {
        // Run iteration
        sw_qps_top(arrivals, true, false, matching, matching_size, stable, false);
        
        // Graduate
        sw_qps_top(arrivals, false, true, matching, matching_size, stable, false);
        
        sizes.push_back(matching_size);
        
        if (cycle < 10 || cycle >= T-2 && cycle < T+2) {
            cout << "  Cycle " << cycle << ": matching size = " << (int)matching_size << endl;
        }
    }
    
    // Should see matches appear after window fills
    int early_matches = accumulate(sizes.begin(), sizes.begin() + T/2, 0);
    int late_matches = accumulate(sizes.begin() + T, sizes.end(), 0);
    
    cout << "Early matches (0-" << T/2 << "): " << early_matches << endl;
    cout << "Late matches (" << T << "-" << 2*T << "): " << late_matches << endl;
    
    assert(late_matches > early_matches);  // More matches after window fills
    cout << "✓ Sliding window test passed" << endl;
}

int run_sw_qps_pure_suite() {
    cout << "========================================" << endl;
    cout << "SW-QPS PURE C++ TESTBENCH" << endl;
    cout << "========================================" << endl;
    cout << "Configuration:" << endl;
    cout << "  N = " << N << " ports" << endl;
    cout << "  T = " << T << " time slots" << endl;
    cout << "  Knockout = " << KNOCKOUT_THRESH << endl;
    cout << endl;
    
    try {
        // Run tests
        test_basic_matching();
        test_no_conflicts();
        test_single_cycle_interface();
        test_sliding_window();
        
        // Test different traffic patterns at various loads
        vector<double> test_loads = {0.3, 0.5, 0.7, 0.9};
        
        for (double load : test_loads) {
            UniformTraffic uniform(load);
            test_traffic_pattern(&uniform, 1000);
            
            DiagonalTraffic diagonal(load);
            test_traffic_pattern(&diagonal, 1000);
            
            QuasiDiagonalTraffic quasi(load);
            test_traffic_pattern(&quasi, 1000);
        }
        
        cout << "\n========================================" << endl;
        cout << "ALL TESTS PASSED!" << endl;
        cout << "========================================" << endl;
        
    } catch (const exception& e) {
        cerr << "Test failed: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}

#ifndef SW_QPS_PURE_DISABLE_MAIN
int main() {
    return run_sw_qps_pure_suite();
}
#endif
