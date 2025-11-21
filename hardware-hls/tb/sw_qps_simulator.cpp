#include "sw_qps_simulator.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>

// ============================================================================
// SW-QPS ALGORITHM IMPLEMENTATION
// ============================================================================

class SWQPSScheduler {
private:
    std::mt19937 rng;
    SlidingWindow window;
    std::vector<VOQState> input_voqs;  // VOQs at each input port
    std::vector<uint16_t> input_availability;  // Availability bitmap for each input
    
    // Performance tracking
    std::vector<int> matching_sizes;
    long long total_matches;
    
public:
    SWQPSScheduler(unsigned seed = 42) : 
        rng(seed), input_voqs(N), input_availability(N, (1 << T) - 1) {
        total_matches = 0;
    }
    
    // Queue-Proportional Sampling: Sample output port based on VOQ lengths
    int qpsSample(int input_port) {
        auto& voq_state = input_voqs[input_port];
        
        if (voq_state.total_packets == 0) {
            return INVALID_PORT;
        }
        
        // Generate random number in [0, total_packets)
        std::uniform_int_distribution<int> dist(0, voq_state.total_packets - 1);
        int target = dist(rng);
        
        // Find which VOQ was sampled
        int cumsum = 0;
        for (int output = 0; output < N; output++) {
            cumsum += voq_state.voq_lengths[output];
            if (target < cumsum) {
                return output;
            }
        }
        
        return N - 1;  // Shouldn't reach here
    }
    
    // First Fit Accept: Find earliest mutual availability slot
    int firstFitAccept(uint16_t input_avail, uint16_t output_avail) {
        uint16_t mutual = input_avail & output_avail;
        if (mutual == 0) return INVALID_PORT;
        
        // Find first set bit (earliest available slot)
        return __builtin_ctz(mutual);
    }
    
    // Run one iteration of SW-QPS
    void runIteration() {
        // Phase 1: Proposing
        std::vector<std::vector<Proposal>> proposals(N);  // Proposals for each output
        
        for (int input = 0; input < N; input++) {
            // Sample an output port using QPS
            int output = qpsSample(input);
            
            if (output != INVALID_PORT && input_voqs[input].voq_lengths[output] > 0) {
                proposals[output].push_back(
                    Proposal(input, input_voqs[input].voq_lengths[output], 
                            input_availability[input])
                );
            }
        }
        
        // Phase 2: Accepting (with knockout if needed)
        for (int output = 0; output < N; output++) {
            auto& output_proposals = proposals[output];
            
            if (output_proposals.empty()) continue;
            
            // Sort proposals by VOQ length (descending)
            std::sort(output_proposals.begin(), output_proposals.end(),
                     [](const Proposal& a, const Proposal& b) {
                         return a.voq_length > b.voq_length;
                     });
            
            // Apply knockout threshold
            int num_to_process = std::min(KNOCKOUT_THRESH, 
                                         static_cast<int>(output_proposals.size()));
            
            // Try to accept proposals using First Fit Accept
            for (int i = 0; i < num_to_process; i++) {
                const auto& prop = output_proposals[i];
                
                // Find first mutual available slot
                int slot = firstFitAccept(prop.availability_bitmap,
                                        window.calendars[output].availability_bitmap);
                
                if (slot != INVALID_PORT) {
                    // Accept the proposal
                    window.calendars[output].schedule[slot] = prop.input_port;
                    window.calendars[output].markSlotUnavailable(slot);
                    input_availability[prop.input_port] &= ~(1 << slot);
                    break;  // Only accept one proposal per output per iteration
                }
            }
        }
    }
    
    // Graduate current matching and process packets
    std::vector<std::pair<int, int>> graduate() {
        auto matching = window.graduate();
        
        // Update availability bitmaps after graduation
        for (int i = 0; i < N; i++) {
            // Shift availability and add new slot
            input_availability[i] = ((input_availability[i] << 1) | 1) & ((1 << T) - 1);
        }
        
        // Track matching size
        matching_sizes.push_back(matching.size());
        total_matches += matching.size();
        
        return matching;
    }
    
    // Add packet to VOQ
    void addPacket(int input_port, int output_port, std::shared_ptr<Packet> packet) {
        input_voqs[input_port].voqs[input_port][output_port].push(packet);
        input_voqs[input_port].voq_lengths[output_port]++;
        input_voqs[input_port].total_packets++;
    }
    
    // Remove packet from VOQ
    std::shared_ptr<Packet> removePacket(int input_port, int output_port) {
        if (input_voqs[input_port].voq_lengths[output_port] > 0) {
            auto packet = input_voqs[input_port].voqs[input_port][output_port].front();
            input_voqs[input_port].voqs[input_port][output_port].pop();
            input_voqs[input_port].voq_lengths[output_port]--;
            input_voqs[input_port].total_packets--;
            return packet;
        }
        return nullptr;
    }
    
    // Get current VOQ statistics
    void getVOQStats(double& mean_length, double& max_length) const {
        mean_length = 0.0;
        max_length = 0.0;
        int count = 0;
        
        for (const auto& input_voq : input_voqs) {
            for (int len : input_voq.voq_lengths) {
                mean_length += len;
                max_length = std::max(max_length, static_cast<double>(len));
                count++;
            }
        }
        
        if (count > 0) {
            mean_length /= count;
        }
    }
    
    // Get matching statistics
    void getMatchingStats(double& mean_size, double& efficiency) const {
        if (matching_sizes.empty()) {
            mean_size = 0.0;
            efficiency = 0.0;
            return;
        }
        
        mean_size = std::accumulate(matching_sizes.begin(), matching_sizes.end(), 0.0) 
                    / matching_sizes.size();
        efficiency = mean_size / N;
    }
    
    // Check if switch is stable (VOQs not growing unboundedly)
    bool isStable() const {
        for (const auto& input_voq : input_voqs) {
            for (int len : input_voq.voq_lengths) {
                if (len > MAX_VOQ_LEN / 2) {  // Half of max as threshold
                    return false;
                }
            }
        }
        return true;
    }
};

// ============================================================================
// SIMULATION ENGINE
// ============================================================================

class NetworkSimulator {
private:
    std::unique_ptr<TrafficGenerator> traffic_gen;
    std::unique_ptr<SWQPSScheduler> scheduler;
    std::mt19937 traffic_rng;
    
    // Packet tracking
    int next_packet_id;
    std::vector<std::shared_ptr<Packet>> all_packets;
    std::vector<int> packet_delays;
    
    // Statistics
    long long packets_arrived;
    long long packets_departed;
    int current_time;
    
public:
    NetworkSimulator(TrafficPattern pattern, unsigned seed = 42) : 
        traffic_rng(seed), next_packet_id(0), 
        packets_arrived(0), packets_departed(0), current_time(0) {
        
        // Initialize traffic generator based on pattern
        switch (pattern) {
            case TrafficPattern::UNIFORM:
                traffic_gen = std::make_unique<UniformTraffic>();
                break;
            case TrafficPattern::QUASI_DIAGONAL:
                traffic_gen = std::make_unique<QuasiDiagonalTraffic>();
                break;
            case TrafficPattern::LOG_DIAGONAL:
                traffic_gen = std::make_unique<LogDiagonalTraffic>();
                break;
            case TrafficPattern::DIAGONAL:
                traffic_gen = std::make_unique<DiagonalTraffic>();
                break;
            case TrafficPattern::HOTSPOT:
                traffic_gen = std::make_unique<HotspotTraffic>(0, 0.5);
                break;
            default:
                traffic_gen = std::make_unique<UniformTraffic>();
        }
        
        scheduler = std::make_unique<SWQPSScheduler>(seed + 1);
    }
    
    // Run simulation for specified time and load
    PerformanceMetrics simulate(double offered_load, int simulation_time, 
                                int warmup_time = 10000, bool verbose = false) {
        reset();
        
        if (verbose) {
            std::cout << "\nStarting simulation: " << traffic_gen->getName() 
                     << " traffic, load=" << offered_load 
                     << ", time=" << simulation_time << " slots" << std::endl;
        }
        
        // Main simulation loop
        for (current_time = 0; current_time < simulation_time + warmup_time; current_time++) {
            // Progress indicator
            if (verbose && current_time % 10000 == 0) {
                std::cout << "  Time: " << current_time << "/" 
                         << (simulation_time + warmup_time) << "\r" << std::flush;
            }
            
            // Step 1: Generate new packet arrivals
            generateArrivals(offered_load);
            
            // Step 2: Run SW-QPS iteration
            scheduler->runIteration();
            
            // Step 3: Graduate matching and switch packets
            auto matching = scheduler->graduate();
            processMatching(matching);
            
            // Check stability
            if (current_time > warmup_time && current_time % 1000 == 0) {
                if (!scheduler->isStable()) {
                    if (verbose) {
                        std::cout << "\nWarning: System appears unstable at time " 
                                 << current_time << std::endl;
                    }
                    break;
                }
            }
        }
        
        if (verbose) {
            std::cout << std::endl;
        }
        
        // Calculate and return metrics
        return calculateMetrics(offered_load, simulation_time, warmup_time);
    }
    
    // Run load sweep to find maximum stable throughput
    std::vector<PerformanceMetrics> loadSweep(
        const std::vector<double>& loads,
        int simulation_time = 100000,
        int warmup_time = 10000,
        bool verbose = true) {
        
        std::vector<PerformanceMetrics> results;
        
        for (double load : loads) {
            if (verbose) {
                std::cout << "\n==== Load = " << load << " ====" << std::endl;
            }
            
            auto metrics = simulate(load, simulation_time, warmup_time, verbose);
            results.push_back(metrics);
            
            if (verbose) {
                metrics.printSummary();
            }
        }
        
        return results;
    }
    
private:
    void reset() {
        scheduler = std::make_unique<SWQPSScheduler>(traffic_rng());
        next_packet_id = 0;
        all_packets.clear();
        packet_delays.clear();
        packets_arrived = 0;
        packets_departed = 0;
        current_time = 0;
    }
    
    void generateArrivals(double load) {
        for (int input = 0; input < N; input++) {
            if (traffic_gen->shouldGeneratePacket(input, load, traffic_rng)) {
                int output = traffic_gen->selectOutputPort(input, traffic_rng);
                
                auto packet = std::make_shared<Packet>(
                    input, output, current_time, next_packet_id++
                );
                
                scheduler->addPacket(input, output, packet);
                all_packets.push_back(packet);
                packets_arrived++;
            }
        }
    }
    
    void processMatching(const std::vector<std::pair<int, int>>& matching) {
        for (const auto& [input, output] : matching) {
            auto packet = scheduler->removePacket(input, output);
            if (packet) {
                packet->departure_time = current_time;
                packets_departed++;
                
                // Only count delays after warmup
                if (current_time >= 10000) {  // Assuming default warmup
                    packet_delays.push_back(packet->getDelay());
                }
            }
        }
    }
    
    PerformanceMetrics calculateMetrics(double offered_load, int simulation_time, 
                                       int warmup_time) {
        PerformanceMetrics metrics;
        
        // Basic info
        metrics.offered_load = offered_load;
        metrics.traffic_pattern = traffic_gen->getName();
        metrics.simulation_time = simulation_time;
        
        // Throughput metrics (only count after warmup)
        int effective_time = std::min(current_time - warmup_time, simulation_time);
        if (effective_time > 0) {
            // Count packets after warmup
            long long warmup_arrived = static_cast<long long>(warmup_time * offered_load * N);
            long long warmup_departed = warmup_arrived;  // Approximate
            
            metrics.total_packets_arrived = packets_arrived - warmup_arrived;
            metrics.total_packets_departed = packets_departed - warmup_departed;
            metrics.throughput = static_cast<double>(metrics.total_packets_departed) / effective_time;
            metrics.normalized_throughput = metrics.throughput / N;
        }
        
        // Delay metrics
        if (!packet_delays.empty()) {
            metrics.calculateDelayPercentiles(packet_delays);
        }
        
        // VOQ statistics
        scheduler->getVOQStats(metrics.mean_voq_length, metrics.max_voq_length);
        
        // Matching statistics
        scheduler->getMatchingStats(metrics.mean_matching_size, metrics.matching_efficiency);
        
        return metrics;
    }
};

// ============================================================================
// COMPARISON WITH OTHER ALGORITHMS
// ============================================================================

// Simple iSLIP implementation for comparison
class iSLIPScheduler {
private:
    std::vector<int> input_pointers;   // Round-robin pointers for inputs
    std::vector<int> output_pointers;  // Round-robin pointers for outputs
    std::vector<VOQState> voqs;
    int iterations;
    
public:
    iSLIPScheduler(int iters = 4) : 
        input_pointers(N, 0), output_pointers(N, 0), voqs(N), iterations(iters) {}
    
    std::vector<std::pair<int, int>> computeMatching() {
        std::vector<std::pair<int, int>> matching;
        std::vector<bool> input_matched(N, false);
        std::vector<bool> output_matched(N, false);
        
        for (int iter = 0; iter < iterations; iter++) {
            // Request phase
            std::vector<std::vector<int>> requests(N);
            for (int input = 0; input < N; input++) {
                if (!input_matched[input]) {
                    for (int output = 0; output < N; output++) {
                        if (!output_matched[output] && voqs[input].voq_lengths[output] > 0) {
                            requests[output].push_back(input);
                        }
                    }
                }
            }
            
            // Grant phase
            std::vector<int> grants(N, INVALID_PORT);
            for (int output = 0; output < N; output++) {
                if (!output_matched[output] && !requests[output].empty()) {
                    // Start from pointer position
                    int start = output_pointers[output];
                    for (int i = 0; i < N; i++) {
                        int input = (start + i) % N;
                        if (std::find(requests[output].begin(), requests[output].end(), input) 
                            != requests[output].end()) {
                            grants[output] = input;
                            break;
                        }
                    }
                }
            }
            
            // Accept phase
            for (int output = 0; output < N; output++) {
                if (grants[output] != INVALID_PORT) {
                    int input = grants[output];
                    if (!input_matched[input]) {
                        matching.push_back({input, output});
                        input_matched[input] = true;
                        output_matched[output] = true;
                        
                        // Update pointers
                        input_pointers[input] = (output + 1) % N;
                        output_pointers[output] = (input + 1) % N;
                    }
                }
            }
        }
        
        return matching;
    }
    
    void addPacket(int input, int output, std::shared_ptr<Packet> packet) {
        voqs[input].voqs[input][output].push(packet);
        voqs[input].voq_lengths[output]++;
        voqs[input].total_packets++;
    }
    
    std::shared_ptr<Packet> removePacket(int input, int output) {
        if (voqs[input].voq_lengths[output] > 0) {
            auto packet = voqs[input].voqs[input][output].front();
            voqs[input].voqs[input][output].pop();
            voqs[input].voq_lengths[output]--;
            voqs[input].total_packets--;
            return packet;
        }
        return nullptr;
    }
};

// QPS-1 (single iteration QPS) for comparison
class QPS1Scheduler {
private:
    std::mt19937 rng;
    std::vector<VOQState> voqs;
    
public:
    QPS1Scheduler(unsigned seed = 42) : rng(seed), voqs(N) {}
    
    std::vector<std::pair<int, int>> computeMatching() {
        std::vector<std::pair<int, int>> matching;
        std::vector<bool> input_matched(N, false);
        std::vector<bool> output_matched(N, false);
        
        // Single QPS iteration
        std::vector<std::vector<std::pair<int, int>>> proposals(N);
        
        // Proposing phase
        for (int input = 0; input < N; input++) {
            if (voqs[input].total_packets > 0) {
                // QPS sampling
                std::uniform_int_distribution<int> dist(0, voqs[input].total_packets - 1);
                int target = dist(rng);
                int cumsum = 0;
                
                for (int output = 0; output < N; output++) {
                    cumsum += voqs[input].voq_lengths[output];
                    if (target < cumsum) {
                        proposals[output].push_back({input, voqs[input].voq_lengths[output]});
                        break;
                    }
                }
            }
        }
        
        // Accepting phase
        for (int output = 0; output < N; output++) {
            if (!proposals[output].empty()) {
                // Accept proposal with longest VOQ
                auto best = std::max_element(proposals[output].begin(), proposals[output].end(),
                    [](const auto& a, const auto& b) { return a.second < b.second; });
                
                matching.push_back({best->first, output});
                input_matched[best->first] = true;
                output_matched[output] = true;
            }
        }
        
        return matching;
    }
    
    void addPacket(int input, int output, std::shared_ptr<Packet> packet) {
        voqs[input].voqs[input][output].push(packet);
        voqs[input].voq_lengths[output]++;
        voqs[input].total_packets++;
    }
    
    std::shared_ptr<Packet> removePacket(int input, int output) {
        if (voqs[input].voq_lengths[output] > 0) {
            auto packet = voqs[input].voqs[input][output].front();
            voqs[input].voqs[input][output].pop();
            voqs[input].voq_lengths[output]--;
            voqs[input].total_packets--;
            return packet;
        }
        return nullptr;
    }
};

// ============================================================================
// MAIN TEST PROGRAM
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "SW-QPS NETWORK SIMULATOR" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  N = " << N << " ports" << std::endl;
    std::cout << "  T = " << T << " time slots (window size)" << std::endl;
    std::cout << "  Knockout = " << KNOCKOUT_THRESH << std::endl;
    
    // Test parameters
    std::vector<double> test_loads = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.85, 0.9, 0.95, 0.99};
    int simulation_time = 100000;
    int warmup_time = 10000;
    
    // Results storage
    std::string results_file = "sw_qps_results.csv";
    
    // Test all traffic patterns
    std::vector<TrafficPattern> patterns = {
        TrafficPattern::UNIFORM,
        TrafficPattern::QUASI_DIAGONAL,
        TrafficPattern::LOG_DIAGONAL,
        TrafficPattern::DIAGONAL,
        TrafficPattern::HOTSPOT
    };
    
    for (auto pattern : patterns) {
        std::cout << "\n========================================" << std::endl;
        NetworkSimulator sim(pattern);
        
        auto results = sim.loadSweep(test_loads, simulation_time, warmup_time, true);
        
        // Save results
        for (const auto& metrics : results) {
            metrics.saveToCSV(results_file);
        }
        
        // Find maximum stable throughput
        double max_stable_throughput = 0.0;
        for (const auto& metrics : results) {
            if (metrics.normalized_throughput > 0.95 * metrics.offered_load) {
                max_stable_throughput = metrics.normalized_throughput;
            }
        }
        
        std::cout << "\nMaximum stable throughput: " 
                 << max_stable_throughput * 100 << "%" << std::endl;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Simulation complete. Results saved to " << results_file << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
