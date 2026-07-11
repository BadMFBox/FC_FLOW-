#include "fc_flow/crypto/crossbolt.hpp"
#include "fc_flow/enforcement/monitus.hpp"
#include "fc_flow/burner/token.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <sched.h>
#include <cstring>

using namespace fc_flow;

// Read current CPU frequency
uint64_t read_cpu_freq(int cpu) {
    std::ifstream freq_file("/sys/devices/system/cpu/cpu" + 
                            std::to_string(cpu) + 
                            "/cpufreq/scaling_cur_freq");
    uint64_t freq = 0;
    freq_file >> freq;
    return freq;
}

int main() {
    const int TARGET_CORE = 4;
    
    // Pin to core 4
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(TARGET_CORE, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    
    std::cout << "========================================================\n";
    std::cout << "   FC_FLOW BENCH WITH GOVERNOR STATE LOGGING           \n";
    std::cout << "========================================================\n\n";
    
    // Open log file
    std::ofstream log("governor_correlation.csv");
    log << "iteration,latency_us,cpu_freq_khz,timestamp_ns\n";
    
    // Initialize crypto engine
    crypto::CrossBolt crossbolt;
    
    // Generate test key
    std::vector<uint8_t> master_key(32);
    for (size_t i = 0; i < 32; ++i) {
        master_key[i] = static_cast<uint8_t>(i);
    }
    
    std::cout << "[*] Running 1000 HMAC operations with frequency logging...\n\n";
    
    for (int i = 0; i < 1000; ++i) {
        // Sample frequency BEFORE operation
        uint64_t freq_before = read_cpu_freq(TARGET_CORE);
        
        // Create input data
        std::string sector_id = "sector_" + std::to_string(i % 10);
        std::vector<uint8_t> input(sector_id.begin(), sector_id.end());
        
        // Run HMAC with timing
        auto start = std::chrono::high_resolution_clock::now();
        
        auto hmac_result = crossbolt.hmac_sha256(master_key, input);
        
        auto end = std::chrono::high_resolution_clock::now();
        
        if (hmac_result.empty()) {
            std::cerr << "✗ HMAC failed at iteration " << i << "\n";
            return 1;
        }
        
        // Calculate latency
        auto duration = std::chrono::duration<double, std::micro>(end - start).count();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            start.time_since_epoch()).count();
        
        // Log: iteration, latency, frequency, timestamp
        log << i << "," << duration << "," << freq_before << "," << timestamp << "\n";
        
        if ((i + 1) % 100 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/1000 operations\n";
        }
    }
    
    log.close();
    
    std::cout << "\n========================================================\n";
    std::cout << "✓ Data logged to: governor_correlation.csv\n";
    std::cout << "\nAnalysis:\n";
    std::cout << "  1. Find rows with high latency (>50 μs for HMAC)\n";
    std::cout << "  2. Check if cpu_freq_khz dropped during those iterations\n";
    std::cout << "  3. If freq drops correlate with jitter: WALT is throttling\n";
    std::cout << "========================================================\n";
    
    return 0;
}
