#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>       // ← ADD THIS
#include "fc_flow/burner/token.hpp"

namespace fc_flow::burner {

// ═══════════════════════════════════════════════════════
// SLINGSHOT VALIDATOR
// Exploit WALT ramp as acceleration curve
// Buffer at 1.7 GHz → batch while ramping → 
// hit 2.8 GHz → BLAST through validations
// ═══════════════════════════════════════════════════════

class SlingshotValidator {
public:
    SlingshotValidator(int target_core);
    ~SlingshotValidator();

    // Queue token for validation
    void queue_token(const Token& token);

    // Start the slingshot loop
    void ignite();
    void shutdown();

    // Stats
    uint64_t tokens_validated() const { return validated_count_; }
    uint64_t avg_latency_us()   const;

private:
    int                        target_core_;
    std::atomic<bool>          running_;
    std::thread                worker_;
    
    std::queue<Token>          queue_;
    std::mutex                 queue_lock_;
    
    std::atomic<uint64_t>      validated_count_;
    std::vector<uint64_t>      latencies_us_;
    mutable std::mutex         stats_lock_;

    // Main loop
    void worker_loop();
    
    // Pre-heat: trigger WALT ramp without blocking
    void preheat_core();
    
    // Batch validation at 2.8 GHz
    void validate_batch(std::vector<Token>& batch);
    
    // Frequency monitor
    uint64_t read_cpu_freq() const;
    
    void log(const std::string& msg);
};

} // namespace fc_flow::burner
