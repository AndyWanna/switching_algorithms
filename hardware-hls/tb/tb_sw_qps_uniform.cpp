/*
 * ============================================================================
 * SW-QPS HLS CO-SIMULATION TESTBENCH - UNIFORM TRAFFIC
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
        double normalized_throughput = throughput / (offered_load * N);
        double avg_matching_size = matching_sizes.empty() ? 0 :
            accumulate(matching_sizes.begin(), matching_sizes.end(), 0.0) / matching_sizes.size();
        double matching_efficiency = avg_matching_size / N;
        double avg_voq_length = (voq_samples > 0) ? sum_voq_lengths / voq_samples / (N*N) : 0;
        
        cout << "\nThroughput Metrics:" << endl;
        cout << "  Packets Arrived: " << total_packets_arrived << endl;
        cout << "  Packets Departed: " << total_packets_departed << endl;
        cout << "  Throughput: " << throughput << " packets/cycle" << endl;
        cout << "  Normalized Throughput: " << normalized_throughput * 100 << "%" << endl;
        
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
            double normalized = throughput / (load * N);
            double avg_match = matching_sizes.empty() ? 0 :
                accumulate(matching_sizes.begin(), matching_sizes.end(), 0.0) / matching_sizes.size();
            double efficiency = avg_match / N;
            double avg_voq = (voq_samples > 0) ? sum_voq_lengths / voq_samples / (N*N) : 0;
            
            file << pattern << "," << load << "," << throughput << ","
                 << normalized << "," << avg_match << "," << efficiency << ","
                 << max_voq_length << "," << avg_voq << endl;
            
            file.close();
        }
    }
};

// Bernoulli traffic generator - UNIFORM ONLY
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
            arrivals[i].output_port = port_dist(rng);
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
    
    // Track VOQ lengths locally
    queue_len_t voq_lengths[N][N];
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            voq_lengths[i][j] = 0;
        }
    }
    
    // Reset system
    sw_qps_top(arrivals, false, false, matching, matching_size, stable, true);
    
    // Main simulation loop
    int total_time = warmup_time + simulation_time;
    
    for (int cycle = 0; cycle < total_time; cycle++) {
        // Progress indicator
        if (verbose && cycle % 1000 == 0) {
            cout << "  Cycle: " << cycle << "/" << total_time << "\r" << flush;
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
        
        // Run iteration
        sw_qps_top(arrivals, true, false, matching, matching_size, stable, false);
        
        // Graduate matching
        sw_qps_top(arrivals, false, true, matching, matching_size, stable, false);
        
        // Update local VOQ state based on departures
        int actual_departures = 0;
        for (int out = 0; out < N; out++) {
            if (matching[out] != INVALID_PORT) {
                int in = matching[out];
                if (voq_lengths[in][out] > 0) {
                    voq_lengths[in][out]--;
                    actual_departures++;
                }
            }
        }
        
        // Record statistics after warmup
        if (cycle >= warmup_time) {
            monitor.recordArrivals(arrival_count);
            monitor.recordMatching(actual_departures);
            monitor.total_cycles++;
            
            if (cycle % 100 == 0) {
                monitor.recordVOQState(voq_lengths);
            }
        }
    }
    
    if (verbose) cout << endl;
    
    // Print results
    monitor.printSummary(offered_load, pattern);
    
    // Save to CSV
    monitor.saveToCSV("sw_qps_uniform_results.csv", offered_load, pattern);
}

int main() {
    cout << "========================================" << endl;
    cout << "SW-QPS HLS - UNIFORM TRAFFIC" << endl;
    cout << "========================================" << endl;
    cout << "Configuration:" << endl;
    cout << "  N = " << N << " ports" << endl;
    cout << "  T = " << T << " window size" << endl;
    cout << "  Knockout = " << KNOCKOUT_THRESH << endl;
    cout << endl;
    
    // Test parameters
    // Test parameters
    int simulation_time = 500;
    int warmup_time = 50;
    
    string pattern = "uniform";
    vector<double> loads = {0.1, 0.3, 0.5, 0.7, 0.8, 0.9, 0.95};
    
    // Run tests
    for (double load : loads) {
        testSWQPS(pattern, load, simulation_time, warmup_time, false);
    }
    
    cout << "\n========================================" << endl;
    cout << "UNIFORM TRAFFIC TESTS COMPLETED!" << endl;
    cout << "Results saved to: sw_qps_uniform_results.csv" << endl;
    cout << "========================================" << endl;
    
    return 0;
}
