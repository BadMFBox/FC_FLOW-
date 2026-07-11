#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <sched.h>

uint64_t read_freq(int cpu) {
    std::ifstream f("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cpufreq/scaling_cur_freq");
    uint64_t freq = 0;
    f >> freq;
    return freq;
}

int main() {
    const int TARGET_CORE = 4;
    
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(TARGET_CORE, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    
    std::cout << "========================================================\n";
    std::cout << "   SIMPLE FREQUENCY OBSERVATION TEST                   \n";
    std::cout << "========================================================\n\n";
    
    std::cout << "Monitoring CPU 4 frequency for 10 seconds...\n";
    std::cout << "Spinning at 100% load to trigger WALT ramp-up\n\n";
    
    auto start_time = std::chrono::steady_clock::now();
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        
        if (elapsed >= 10) break;
        
        // Sample frequency
        uint64_t freq = read_freq(TARGET_CORE);
        
        std::cout << "t=" << elapsed << "s: " << freq << " kHz\n";
        
        // Spin for ~1 second at 100% CPU
        auto spin_start = std::chrono::high_resolution_clock::now();
        volatile uint64_t counter = 0;
        while (std::chrono::high_resolution_clock::now() - spin_start < std::chrono::seconds(1)) {
            counter++;
        }
    }
    
    std::cout << "\n========================================================\n";
    std::cout << "Expected behavior with hispeed_freq=1344000 + load=90%:\n";
    std::cout << "  - Freq should ramp from ~1.7 GHz → 2.8 GHz\n";
    std::cout << "  - If it stays at 1.3-1.7 GHz: WALT is blocking ramp\n";
    std::cout << "========================================================\n";
    
    return 0;
}
