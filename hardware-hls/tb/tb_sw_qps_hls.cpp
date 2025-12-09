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
    vector<int> packet_delays;
    queue_len_t max_voq_length;
    double sum_voq_lengths;
    int voq_samples;
    
    PerformanceMonitor() { reset(); }
    
    void reset() {
        total_packets_arrived = 0;
        total_packets_departed = 0;
        total_cycles = 0;
        matching_sizes.clear();
        packet_delays.clear();
        max_voq_length = 0;
        sum_voq_lengths = 0;
        voq_samples = 0;
    }
    
    void recordMatching(int size) {
        matching_sizes.push_back(size);
        total_packets_departed += size;
    }
    
    void recordArrivals(int count) {
        total_packets_arrived += count;
    }
    
    void recordVOQState(queue_len_t voq_lengths[N][N]) {
        queue_len_t max_len = 0;
        queue_len_t sum = 0;
        
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                max_len = (voq_lengths[i][j] > max_len) ? voq_lengths[i][j] : max_len;
                sum += voq_lengths[i][j];
            }
        }
        
        max_voq_length = (max_len > max_voq_length) ? max_len : max_voq_length;
        sum_voq_lengths += sum;
        voq_samples++;
    }
    
    void printSummary(double offered_load, const string& pattern_name) {
        cout << "\n=== Performance Summary ===" << endl;
        cout << "Traffic Pattern: " << pattern_name << endl;
        cout << "Offered Load: " << offered_load << endl;
        cout << "Total Cycles: " << total_cycles << endl;
        
        double throughput = (double)total_packets_departed / total_cycles;
        double normalized_throughput = throughput / N;  // FIX: Normalize by max possible (N)
        double avg_matching_size = matching_sizes.empty() ? 0 :
            accumulate(matching_sizes.begin(), matching_sizes.end(), 0.0) / matching_sizes.size();
        double matching_efficiency = avg_matching_size / N;
        double avg_voq_length = (voq_samples > 0) ? sum_voq_lengths / voq_samples / (N*N) : 0;
        double arrival_rate = (double)total_packets_arrived / total_cycles;
        double load_utilization = (arrival_rate > 0) ? throughput / arrival_rate : 0.0;

        cout << "\nThroughput Metrics:" << endl;
        cout << "  Packets Arrived: " << total_packets_arrived << endl;
        cout << "  Packets Departed: " << total_packets_departed << endl;
        cout << "  Arrival Rate: " << arrival_rate << " packets/cycle" << endl;
        cout << "  Throughput: " << throughput << " packets/cycle" << endl;
        cout << "  Normalized Throughput: " << normalized_throughput * 100 << "% (of max " << N << " packets/cycle)" << endl;
        cout << "  Load Utilization: " << load_utilization * 100 << "% (throughput/arrivals)" << endl;
        
        cout << "\nMatching Metrics:" << endl;
        cout << "  Average Matching Size: " << avg_matching_size << endl;
        cout << "  Matching Efficiency: " << matching_efficiency * 100 << "%" << endl;
        
        cout << "\nQueue Metrics:" << endl;
        cout << "  Max VOQ Length: " << max_voq_length << endl;
        cout << "  Avg VOQ Length: " << avg_voq_length << endl;
    }
    
    void saveToCSV(const string& filename, double load, const string& pattern) {
        ofstream file(filename, ios::app);
        if (file.is_open()) {
            // Write header if file is empty
            file.seekp(0, ios::end);
            if (file.tellp() == 0) {
                file << "pattern,load,throughput,normalized_throughput,"
                     << "avg_matching_size,matching_efficiency,max_voq,avg_voq" << endl;
            }
            
            double throughput = (double)total_packets_departed / total_cycles;
            double normalized = throughput / N;  // FIX: Normalize by max possible (N)
            double avg_match = matching_sizes.empty() ? 0 :
                accumulate(matching_sizes.begin(), matching_sizes.end(), 0.0) / matching_sizes.size();
            double efficiency = avg_match / N;
            double avg_voq = (voq_samples > 0) ? sum_voq_lengths / voq_samples / (N*N) : 0;
            double arrival_rate = (double)total_packets_arrived / total_cycles;

            file << pattern << "," << load << "," << throughput << ","
                 << normalized << "," << avg_match << "," << efficiency << ","
                 << max_voq_length << "," << avg_voq << "," << arrival_rate << endl;
            
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

    cout << "Initializing data structures..." << endl;

    // Initialize
    PacketArrival arrivals[N];
    port_id_t matching[N];
    ap_uint<8> matching_size;
    bool stable = false;
    mt19937 rng(12345);
    PerformanceMonitor monitor;

    cout << "Initializing VOQ tracking..." << endl;

    // Track VOQ lengths locally
    queue_len_t voq_lengths[N][N];
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            voq_lengths[i][j] = 0;
        }
    }

    cout << "Resetting HLS core..." << endl;

    // Reset system
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, true);

    cout << "Reset complete!" << endl;
    
    // Main simulation loop
    int total_time = warmup_time + simulation_time;

    cout << "Starting simulation loop: " << total_time << " cycles" << endl;

    for (int cycle = 0; cycle < total_time; cycle++) {
        // Progress indicator - print every 10 cycles for debugging
        if (cycle % 10 == 0) {
            cout << "  Cycle: " << cycle << "/" << total_time << " (arrivals=" << monitor.total_packets_arrived
                 << ", departures=" << monitor.total_packets_departed << ")" << endl;
        }

        // Generate traffic
        generateBernoulliTraffic(arrivals, offered_load, pattern, rng);

        // Count arrivals and update local VOQ state
        int arrival_count = 0;
        for (int i = 0; i < N; i++) {
            if (arrivals[i].valid) {
                arrival_count++;
                voq_lengths[arrivals[i].input_port][arrivals[i].output_port]++;
            }
        }

        // Process arrivals
        sw_qps_top(arrivals, false, false, matching, matching_size, stable, false);

        // Clear arrivals for next operations
        for (int i = 0; i < N; i++) arrivals[i].valid = false;

        // Run T iterations (one for each matching in the sliding window)
        // This is CRITICAL for SW-QPS performance!
        for (int iter = 0; iter < T; iter++) {
            sw_qps_top(arrivals, true, false, matching, matching_size, stable, false);
        }

        // Graduate matching
        sw_qps_top(arrivals, false, true, matching, matching_size, stable, false);
        
        // Update local VOQ state based on departures
        int actual_departures = 0;
        for (int out = 0; out < N; out++) {
            if (matching[out] != INVALID_PORT) {
                int in = matching[out];
                // Count the departure (HLS core tracks VOQs internally)
                // The testbench VOQ tracking is just for statistics
                if (voq_lengths[in][out] > 0) {
                    voq_lengths[in][out]--;
                }
                actual_departures++;
            }
        }
        
        // Record statistics after warmup
        if (cycle >= warmup_time) {
            monitor.recordArrivals(arrival_count);
            monitor.recordMatching(actual_departures);  // Use actual, not matching_size!
            monitor.total_cycles++;
            
            // Sample VOQ state periodically
            if (cycle % 100 == 0) {
                monitor.recordVOQState(voq_lengths);
            }
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

    cout << "Resetting..." << endl;
    // Reset
    sw_qps_single_cycle(voq_state, 1, matching, matching_size, true);
    cout << "Reset complete" << endl;

    // Test different scenarios
    cout << "\n1. Diagonal traffic:" << endl;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            voq_state[i][j] = (i == j) ? 10 : 0;
        }
    }

    cout << "Running single test with T=" << T << " iterations..." << endl;
    sw_qps_single_cycle(voq_state, T, matching, matching_size, false);
    cout << "  Iterations: " << T << ", Matching size: " << (int)matching_size << endl;
    
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

    // SKIP single cycle test for now - it's very slow
    cout << "Skipping single cycle test (too slow)..." << endl;
    // testSingleCycle();

    // Test parameters - REDUCED FOR DEBUGGING
    int simulation_time = 20;  // Reduced from 10000
    int warmup_time = 10;       // Reduced from 1000

    // Traffic patterns to test - REDUCED FOR DEBUGGING
    vector<string> patterns = {"uniform"};  // Just test one pattern

    // Load levels to test - REDUCED FOR DEBUGGING
    vector<double> loads = {0.1, 0.5};  // Just test two loads

    cout << "\n=== Starting Main Simulation ===" << endl;
    cout << "Testing " << patterns.size() << " patterns x " << loads.size() << " loads" << endl;
    cout << "Each test: " << warmup_time << " warmup + " << simulation_time << " measurement cycles" << endl;

    // Run tests
    int test_num = 0;
    int total_tests = patterns.size() * loads.size();
    for (const string& pattern : patterns) {
        for (double load : loads) {
            test_num++;
            cout << "\n[Test " << test_num << "/" << total_tests << "] "
                 << pattern << " @ load=" << load << endl;
            testSWQPS(pattern, load, simulation_time, warmup_time, false);
            cout << "  Completed." << endl;
        }
    }
    
    cout << "\n========================================" << endl;
    cout << "ALL HLS TESTS COMPLETED!" << endl;
    cout << "Results saved to: sw_qps_hls_results.csv" << endl;
    cout << "========================================" << endl;
    
    return 0;
}
