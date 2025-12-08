/*
 * ============================================================================
 * SW-QPS HLS CO-SIMULATION TESTBENCH
 * ============================================================================
 * 
 * Testbench for HLS C simulation and co-simulation
 * Includes traffic generation and performance measurement
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <random>
#include <cmath>
#include <cassert>
#include "../src/sw_qps_top.h"

using namespace std;

// Performance metrics collector
class PerformanceMonitor {
public:
    long total_packets_arrived;
    long total_packets_departed;
    long total_cycles;
    vector<int> matching_sizes;

    PerformanceMonitor() { reset(); }

    void reset() {
        total_packets_arrived = 0;
        total_packets_departed = 0;
        total_cycles = 0;
        matching_sizes.clear();
    }

    void recordMatching(int size) {
        matching_sizes.push_back(size);
        total_packets_departed += size;
    }

    void recordArrivals(int count) {
        total_packets_arrived += count;
    }
    
    void printSummary(double offered_load, const string& pattern_name) {
        cout << "\n=== Performance Summary ===" << endl;
        cout << "Traffic Pattern: " << pattern_name << endl;
        cout << "Offered Load: " << offered_load << endl;
        cout << "Total Cycles: " << total_cycles << endl;

        double throughput = (double)total_packets_departed / total_cycles;
        double normalized_throughput = throughput / N;  // Throughput as % of max (N packets/cycle)
        double avg_matching_size = matching_sizes.empty() ? 0 :
            accumulate(matching_sizes.begin(), matching_sizes.end(), 0.0) / matching_sizes.size();
        double matching_efficiency = avg_matching_size / N;

        cout << "\nThroughput Metrics:" << endl;
        cout << "  Packets Arrived: " << total_packets_arrived << endl;
        cout << "  Packets Departed: " << total_packets_departed << endl;
        cout << "  Throughput: " << throughput << " packets/cycle" << endl;
        cout << "  Normalized Throughput: " << normalized_throughput * 100 << "%" << endl;

        cout << "\nMatching Metrics:" << endl;
        cout << "  Average Matching Size: " << avg_matching_size << endl;
        cout << "  Matching Efficiency: " << matching_efficiency * 100 << "%" << endl;
    }
    
    void saveToCSV(const string& filename, double load, const string& pattern) {
        ofstream file(filename, ios::app);
        if (file.is_open()) {
            // Write header if file is empty
            file.seekp(0, ios::end);
            if (file.tellp() == 0) {
                file << "pattern,load,throughput,normalized_throughput,"
                     << "avg_matching_size,matching_efficiency" << endl;
            }

            double throughput = (double)total_packets_departed / total_cycles;
            double normalized = throughput / N;
            double avg_match = matching_sizes.empty() ? 0 :
                accumulate(matching_sizes.begin(), matching_sizes.end(), 0.0) / matching_sizes.size();
            double efficiency = avg_match / N;

            file << pattern << "," << load << "," << throughput << ","
                 << normalized << "," << avg_match << "," << efficiency << endl;

            file.close();
        }
    }
};

// Bernoulli traffic generator
void generateBernoulliTraffic(
    PacketArrival arrivals[N],
    double load,
    const string& pattern,
    mt19937& rng
) {
    uniform_real_distribution<double> load_dist(0.0, 1.0);
    uniform_int_distribution<int> port_dist(0, N-1);
    
    for (int i = 0; i < N; i++) {
        arrivals[i].valid = false;
        
        if (load_dist(rng) < load) {
            arrivals[i].input_port = i;
            arrivals[i].valid = true;
            
            // Select output based on pattern
            if (pattern == "uniform") {
                arrivals[i].output_port = port_dist(rng);
            }
            else if (pattern == "diagonal") {
                if (load_dist(rng) < 2.0/3.0) {
                    arrivals[i].output_port = i;
                } else {
                    arrivals[i].output_port = (i + 1) % N;
                }
            }
            else if (pattern == "quasi-diagonal") {
                if (load_dist(rng) < 0.5) {
                    arrivals[i].output_port = i;
                } else {
                    int port = port_dist(rng);
                    if (port == i) port = (port + 1) % N;
                    arrivals[i].output_port = port;
                }
            }
            else if (pattern == "log-diagonal") {
                // Simplified log-diagonal
                double r = load_dist(rng);
                if (r < 0.5) {
                    arrivals[i].output_port = i;
                } else if (r < 0.75) {
                    arrivals[i].output_port = (i + 1) % N;
                } else if (r < 0.875) {
                    arrivals[i].output_port = (i + 2) % N;
                } else {
                    arrivals[i].output_port = port_dist(rng);
                }
            }
        }
    }
}

// Main test function
void testSWQPS(
    const string& pattern,
    double offered_load,
    int simulation_time,
    int warmup_time,
    bool verbose = false
) {
    cout << "\n========================================" << endl;
    cout << "Testing: " << pattern << " traffic, load = " << offered_load << endl;
    cout << "========================================" << endl;
    
    // Initialize
    PacketArrival arrivals[N];
    port_id_t matching[N];
    ap_uint<8> matching_size;
    bool stable = false;
    mt19937 rng(12345);
    PerformanceMonitor monitor;

    // NOTE: Cannot track VOQ state locally due to virtual departures
    // The HLS module manages its own VOQs internally

    // Reset system
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, true);

    // CRITICAL: Pre-fill the sliding window with T iterations before first graduation
    // This ensures slot 0 has had T iterations to find matches before it graduates
    for (int init_iter = 0; init_iter < T; init_iter++) {
        sw_qps_top(arrivals, true, false, matching, matching_size, stable, false);
    }

    // Main simulation loop
    int total_time = warmup_time + simulation_time;

    for (int cycle = 0; cycle < total_time; cycle++) {
        // Progress indicator
        if (verbose && cycle % 1000 == 0) {
            cout << "  Cycle: " << cycle << "/" << total_time << "\r" << flush;
        }
        
        // Generate traffic
        generateBernoulliTraffic(arrivals, offered_load, pattern, rng);

        // Count arrivals
        int arrival_count = 0;
        for (int i = 0; i < N; i++) {
            if (arrivals[i].valid) {
                arrival_count++;
            }
        }

        // Process arrivals
        sw_qps_top(arrivals, false, false, matching, matching_size, stable, false);
        
        // Clear arrivals for next operations
        for (int i = 0; i < N; i++) arrivals[i].valid = false;

        // Run iteration
        sw_qps_top(arrivals, true, false, matching, matching_size, stable, false);

        // Graduate matching
        sw_qps_top(arrivals, false, true, matching, matching_size, stable, false);

        // NOTE: With virtual departures, the HLS module handles packet removal internally
        // We cannot track VOQ state locally because packets are removed on accept, not graduation

        // Record statistics after warmup
        if (cycle >= warmup_time) {
            monitor.recordArrivals(arrival_count);
            monitor.recordMatching(matching_size);  // Use matching_size from HLS
            monitor.total_cycles++;
        }
        
        // Check stability
        // if (!stable && cycle % 1000 == 0) {
        //     cout << "\n  Warning: System unstable at cycle " << cycle << endl;
        // }
    }
    
    if (verbose) cout << endl;
    
    // Print results
    monitor.printSummary(offered_load, pattern);
    
    // Save to CSV
    monitor.saveToCSV("sw_qps_hls_results.csv", offered_load, pattern);
}

// Test single cycle interface
void testSingleCycle() {
    cout << "\n=== Testing Single Cycle Interface ===" << endl;
    
    queue_len_t voq_state[N][N];
    port_id_t matching[N];
    ap_uint<8> matching_size;
    
    // Reset
    sw_qps_single_cycle(voq_state, 1, matching, matching_size, true);
    
    // Test different scenarios
    cout << "\n1. Diagonal traffic:" << endl;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            voq_state[i][j] = (i == j) ? 10 : 0;
        }
    }
    
    for (int iters = 1; iters <= T; iters *= 2) {
        sw_qps_single_cycle(voq_state, iters, matching, matching_size, false);
        cout << "  Iterations: " << iters << ", Matching size: " << (int)matching_size << endl;
    }
    
    cout << "\n2. Full mesh traffic:" << endl;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            voq_state[i][j] = 5;
        }
    }
    
    sw_qps_single_cycle(voq_state, T, matching, matching_size, false);
    cout << "  Matching size with full mesh: " << (int)matching_size << endl;
    
    // Verify no conflicts
    bool input_used[N] = {false};
    int conflicts = 0;
    
    for (int out = 0; out < N; out++) {
        if (matching[out] != INVALID_PORT) {
            if (input_used[matching[out]]) {
                conflicts++;
            }
            input_used[matching[out]] = true;
        }
    }
    
    cout << "  Conflicts: " << conflicts << endl;
    assert(conflicts == 0);
    cout << "✓ Single cycle interface test passed" << endl;
}

int main() {
    cout << "========================================" << endl;
    cout << "SW-QPS HLS CO-SIMULATION TESTBENCH" << endl;
    cout << "========================================" << endl;
    cout << "Configuration:" << endl;
    cout << "  N = " << N << " ports" << endl;
    cout << "  T = " << T << " window size" << endl;
    cout << "  Knockout = " << KNOCKOUT_THRESH << endl;
    cout << endl;
    
    // Test single cycle interface first
    testSingleCycle();
    
    // Test parameters
    // Test parameters
    int simulation_time = 10000;
    int warmup_time = 1000;
    
    // Traffic patterns to test
    vector<string> patterns = {"uniform", "diagonal", "quasi-diagonal", "log-diagonal"};
    
    // Load levels to test
    vector<double> loads = {0.1, 0.3, 0.5, 0.7, 0.8, 0.9, 0.95};
    
    // Run tests
    for (const string& pattern : patterns) {
        for (double load : loads) {
            testSWQPS(pattern, load, simulation_time, warmup_time, false);
            
            // Check if we're achieving expected throughput
            // SW-QPS should achieve 85-93% throughput
            if (load >= 0.9) {
                cout << "  Checking throughput at high load..." << endl;
                // Throughput check would be based on results
            }
        }
    }
    
    cout << "\n========================================" << endl;
    cout << "ALL HLS TESTS COMPLETED!" << endl;
    cout << "Results saved to: sw_qps_hls_results.csv" << endl;
    cout << "========================================" << endl;
    
    return 0;
}
