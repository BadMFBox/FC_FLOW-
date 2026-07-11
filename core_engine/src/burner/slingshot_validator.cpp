#include "slingshot_validator.hpp"
#include <sched.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <mutex>      // ← ADD THIS
#include <numeric>    // ← ADD THIS (for std::accumulate)

namespace fc_flow::burner {

SlingshotValidator::SlingshotValidator(int target_core)
    : target_core_(target_core)
    , running_(false)
    , validated_count_(0)
{
    log("SLINGSHOT VALIDATOR CONSTRUCTED — Core " + 
        std::to_string(target_core_));
}

SlingshotValidator::~SlingshotValidator() {
    shutdown();
}

void SlingshotValidator::queue_token(const Token& token) {
    std::lock_guard<std::mutex> lock(queue_lock_);
    queue_.push(token);
}

void SlingshotValidator::ignite() {
    running_ = true;
    
    worker_ = std::thread([this]() {
        // Pin to target core
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(target_core_, &mask);
        sched_setaffinity(0, sizeof(mask), &mask);
        
        worker_loop();
    });
    
    log("SLINGSHOT IGNITED");
}

void SlingshotValidator::shutdown() {
    if (!running_) return;
    running_ = false;
    if (worker_.joinable()) worker_.join();
    log("SLINGSHOT SHUTDOWN");
}

// ── MAIN WORKER LOOP ─────────────────────────────────

void SlingshotValidator::worker_loop() {
    const size_t BATCH_SIZE = 50;  // Validate 50 at a time
    const auto PREHEAT_INTERVAL = std::chrono::milliseconds(100);
    
    auto last_preheat = std::chrono::steady_clock::now();
    
    while (running_) {
        // Check if queue has tokens
        size_t queue_size;
        {
            std::lock_guard<std::mutex> lock(queue_lock_);
            queue_size = queue_.size();
        }
        
        if (queue_size == 0) {
            // No work — sleep and continue
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            continue;
        }
        
        // PRE-HEAT: Trigger WALT ramp BEFORE pulling batch
        auto now = std::chrono::steady_clock::now();
        if (now - last_preheat > PREHEAT_INTERVAL) {
            preheat_core();
            last_preheat = now;
        }
        
        // Pull batch from queue
        std::vector<Token> batch;
        {
            std::lock_guard<std::mutex> lock(queue_lock_);
            size_t count = std::min(queue_size, BATCH_SIZE);
            for (size_t i = 0; i < count; i++) {
                batch.push_back(queue_.front());
                queue_.pop();
            }
        }
        
        // VALIDATE BATCH (core should be at or near 2.8 GHz now)
        validate_batch(batch);
    }
}

// ── PRE-HEAT CORE ────────────────────────────────────

void SlingshotValidator::preheat_core() {
    // Spin at 100% for 50ms to trigger WALT ramp
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t counter = 0;  // ← REMOVE volatile to fix warning
    
    while (std::chrono::high_resolution_clock::now() - start < 
           std::chrono::milliseconds(50)) {
        counter++;  // Burn cycles
    }
    
    uint64_t freq = read_cpu_freq();
    if (freq < 2400000) {
        // Still ramping — extend preheat
        while (std::chrono::high_resolution_clock::now() - start < 
               std::chrono::milliseconds(100)) {
            counter++;
        }
    }
}

// ── BATCH VALIDATION ─────────────────────────────────

void SlingshotValidator::validate_batch(std::vector<Token>& batch) {
    uint64_t freq_start = read_cpu_freq();
    
    for (auto& token : batch) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // ACTUAL VALIDATION
        // Token doesn't have is_valid() — check fields directly
        bool valid = (token.issued_at > 0 && 
                      token.expires_at > token.issued_at);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<
            std::chrono::microseconds>(end - start).count();
        
        {
            std::lock_guard<std::mutex> lock(stats_lock_);
            latencies_us_.push_back(latency);
        }
        
        validated_count_++;
    }
    
    uint64_t freq_end = read_cpu_freq();
    
    log("BATCH VALIDATED: " + std::to_string(batch.size()) + 
        " tokens | Freq: " + std::to_string(freq_start / 1000) + 
        " → " + std::to_string(freq_end / 1000) + " MHz");
}

// ── FREQ MONITOR ─────────────────────────────────────

uint64_t SlingshotValidator::read_cpu_freq() const {
    std::ifstream freq_file(
        "/sys/devices/system/cpu/cpu" + 
        std::to_string(target_core_) + 
        "/cpufreq/scaling_cur_freq");
    uint64_t freq = 0;
    freq_file >> freq;
    return freq;
}

// ── STATS ────────────────────────────────────────────

uint64_t SlingshotValidator::avg_latency_us() const {
    std::lock_guard<std::mutex> lock(stats_lock_);
    if (latencies_us_.empty()) return 0;
    
    uint64_t sum = std::accumulate(
        latencies_us_.begin(), 
        latencies_us_.end(), 
        0ULL);
    return sum / latencies_us_.size();
}

void SlingshotValidator::log(const std::string& msg) {
    std::cout << "[SLINGSHOT] " << msg << "\n";
}

} // namespace fc_flow::burner
