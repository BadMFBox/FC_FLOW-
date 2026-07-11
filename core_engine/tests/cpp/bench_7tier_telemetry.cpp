#include "fc_flow/enforcement/monitus.hpp"
#include "fc_flow/crypto/crossbolt_lock.hpp"
#include "fc_flow/crypto/crossbolt.hpp"
#include <cassert>
#include <cstdio>
#include <sodium.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <algorithm>
#include <sys/resource.h>

using namespace fc_flow::enforcement;
using namespace fc_flow::crypto;

// ═══════════════════════════════════════════════════════
// Advanced Telemetry Container
// ═══════════════════════════════════════════════════════

struct AdvancedBenchResults {
    // Basic Flow
    uint64_t total_operations;
    uint64_t bolts_fired;
    uint64_t bolts_failed;
    uint64_t crossval_performed;
    double throughput_ops_sec;
    double elapsed_sec;

    // Latency Profiles (in nanoseconds)
    uint64_t latency_p50_ns;
    uint64_t latency_p90_ns;
    uint64_t latency_p99_ns;
    uint64_t latency_p999_ns;
    uint64_t latency_max_ns;
    
    // Jitter Analysis
    uint64_t max_jitter_delta_ns;  // Maximum time gap between validations

    // Kernel Integrity
    long voluntary_context_switches;
    long involuntary_context_switches;
};

// ═══════════════════════════════════════════════════════
// Per-Thread Latency Tracker
// ═══════════════════════════════════════════════════════

struct ThreadLatencyTracker {
    std::vector<uint64_t> latencies;
    uint64_t max_validation_gap_ns{0};
    uint64_t last_validation_time_ns{0};
    
    void reserve_capacity(size_t n) {
        latencies.reserve(n);
    }
    
    void record_latency(uint64_t ns) {
        latencies.push_back(ns);
    }
    
    void record_validation_gap(uint64_t current_time_ns) {
        if (last_validation_time_ns > 0) {
            uint64_t gap = current_time_ns - last_validation_time_ns;
            if (gap > max_validation_gap_ns) {
                max_validation_gap_ns = gap;
            }
        }
        last_validation_time_ns = current_time_ns;
    }
};

// ═══════════════════════════════════════════════════════
// Global State
// ═══════════════════════════════════════════════════════

std::atomic<uint64_t> bolt_fired_count{0};
std::atomic<uint64_t> bolt_failed_count{0};
std::atomic<uint64_t> crossval_performed{0};
std::atomic<bool> stop_flag{false};

ThreadLatencyTracker trackers[7];

// ═══════════════════════════════════════════════════════
// 1-3-5 Cross-Validation Pattern
// ═══════════════════════════════════════════════════════

bool should_crossval(uint64_t loop_count) {
    uint64_t cycle = loop_count % 5;
    return (cycle == 0 || cycle == 2 || cycle == 4);  // 1st, 3rd, 5th
}

// ═══════════════════════════════════════════════════════
// Tier Worker with Latency Tracking
// ═══════════════════════════════════════════════════════

void tier_worker(Monitus& monitus, uint8_t sector_id) {
    ThreadLatencyTracker& tracker = trackers[sector_id];
    tracker.reserve_capacity(5000000);  // Pre-allocate for 5M samples
    
    BoltInput input;
    sodium_memzero(&input, sizeof(input));
    input.sector_id = sector_id;
    input.stack_id = 0;
    input.flow_cycle = 0;
    
    randombytes_buf(input.master_secret.data(), 32);
    randombytes_buf(input.hardware_dna.data(), 64);
    randombytes_buf(input.entropy.data(), 32);
    
    uint64_t loop_count = 0;
    
    while (!stop_flag.load(std::memory_order_relaxed)) {
        auto loop_start = std::chrono::steady_clock::now();
        
        input.flow_cycle = loop_count;
        input.tick_salt = loop_count * 1000;
        
        auto fire_result = fire_crossbolt(input);
        
        if (fire_result.has_value()) {
            CrossBolt& bolt = fire_result.value();
            
            if (should_crossval(loop_count)) {
                // Track validation gap for jitter analysis
                auto now_ns = std::chrono::steady_clock::now().time_since_epoch().count();
                tracker.record_validation_gap(now_ns);
                
                auto verify_result = verify_crossbolt(bolt, input);
                crossval_performed.fetch_add(1, std::memory_order_relaxed);
                
                if (verify_result.has_value()) {
                    monitus.witness_bolt_fired(sector_id, loop_count, bolt);
                    bolt_fired_count.fetch_add(1, std::memory_order_relaxed);
                } else {
                    monitus.witness_bolt_failed(sector_id, loop_count, MonitusEventType::BOLT_FAILED);
                    bolt_failed_count.fetch_add(1, std::memory_order_relaxed);
                }
            } else {
                monitus.witness_bolt_fired(sector_id, loop_count, bolt);
                bolt_fired_count.fetch_add(1, std::memory_order_relaxed);
            }
            
            bolt.wipe();
        } else {
            monitus.witness_bolt_failed(sector_id, loop_count, MonitusEventType::BOLT_FAILED);
            bolt_failed_count.fetch_add(1, std::memory_order_relaxed);
        }
        
        auto loop_end = std::chrono::steady_clock::now();
        auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(loop_end - loop_start).count();
        tracker.record_latency(latency_ns);
        
        loop_count++;
    }
}

// ═══════════════════════════════════════════════════════
// Percentile Calculation
// ═══════════════════════════════════════════════════════

uint64_t calculate_percentile(std::vector<uint64_t>& data, double percentile) {
    if (data.empty()) return 0;
    std::sort(data.begin(), data.end());
    size_t index = static_cast<size_t>(data.size() * percentile);
    if (index >= data.size()) index = data.size() - 1;
    return data[index];
}

// ═══════════════════════════════════════════════════════
// Main Bench
// ═══════════════════════════════════════════════════════

int main() {
    if (sodium_init() < 0) {
        fprintf(stderr, "libsodium initialization failed\n");
        return 1;
    }
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║   FC_FLOW — ADVANCED TELEMETRY BENCH (30 seconds)        ║\n");
    printf("║   Tail Latency + Context Switches + Jitter Analysis      ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    // Capture starting context switches
    struct rusage usage_start;
    getrusage(RUSAGE_SELF, &usage_start);
    
    Monitus monitus;
    
    std::thread workers[7];
    for (uint8_t i = 0; i < 7; i++) {
        workers[i] = std::thread(tier_worker, std::ref(monitus), i);
    }
    
    printf("[Bench] 7 tiers active with 1-3-5 cross-validation pattern\n");
    printf("[Bench] Tracking latency distribution and OS context switches\n");
    printf("[Bench] Running for 30 seconds...\n\n");
    
    auto start = std::chrono::steady_clock::now();
    
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    stop_flag.store(true, std::memory_order_relaxed);
    
    for (auto& w : workers) {
        w.join();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Capture ending context switches
    struct rusage usage_end;
    getrusage(RUSAGE_SELF, &usage_end);
    
    // Aggregate latency data from all threads
    std::vector<uint64_t> all_latencies;
    uint64_t max_jitter = 0;
    
    for (int i = 0; i < 7; i++) {
        all_latencies.insert(all_latencies.end(), 
                            trackers[i].latencies.begin(), 
                            trackers[i].latencies.end());
        if (trackers[i].max_validation_gap_ns > max_jitter) {
            max_jitter = trackers[i].max_validation_gap_ns;
        }
    }
    
    // Build results
    AdvancedBenchResults results;
    results.total_operations = bolt_fired_count.load() + bolt_failed_count.load();
    results.bolts_fired = bolt_fired_count.load();
    results.bolts_failed = bolt_failed_count.load();
    results.crossval_performed = crossval_performed.load();
    results.elapsed_sec = elapsed_ms / 1000.0;
    results.throughput_ops_sec = (results.total_operations * 1000.0) / elapsed_ms;
    
    results.latency_p50_ns = calculate_percentile(all_latencies, 0.50);
    results.latency_p90_ns = calculate_percentile(all_latencies, 0.90);
    results.latency_p99_ns = calculate_percentile(all_latencies, 0.99);
    results.latency_p999_ns = calculate_percentile(all_latencies, 0.999);
    results.latency_max_ns = all_latencies.empty() ? 0 : *std::max_element(all_latencies.begin(), all_latencies.end());
    
    results.max_jitter_delta_ns = max_jitter;
    
    results.voluntary_context_switches = usage_end.ru_nvcsw - usage_start.ru_nvcsw;
    results.involuntary_context_switches = usage_end.ru_nivcsw - usage_start.ru_nivcsw;
    
    // Display results
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                  THROUGHPUT METRICS                       ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Elapsed:           %6.2f sec                          ║\n", results.elapsed_sec);
    printf("║  Total Operations:  %12lu                         ║\n", results.total_operations);
    printf("║  Bolts Fired:       %12lu                         ║\n", results.bolts_fired);
    printf("║  Bolts Failed:      %12lu                         ║\n", results.bolts_failed);
    printf("║  Cross-Validations: %12lu (%.1f%%)                ║\n", 
           results.crossval_performed, 
           (results.crossval_performed * 100.0) / results.total_operations);
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Throughput:        %12.0f ops/sec                ║\n", results.throughput_ops_sec);
    printf("║  Per-Tier:          %12.0f ops/sec                ║\n", results.throughput_ops_sec / 7.0);
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║              TAIL LATENCY DISTRIBUTION                    ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  P50  (median):     %12lu ns  (%6.2f µs)       ║\n", 
           results.latency_p50_ns, results.latency_p50_ns / 1000.0);
    printf("║  P90:               %12lu ns  (%6.2f µs)       ║\n", 
           results.latency_p90_ns, results.latency_p90_ns / 1000.0);
    printf("║  P99:               %12lu ns  (%6.2f µs)       ║\n", 
           results.latency_p99_ns, results.latency_p99_ns / 1000.0);
    printf("║  P99.9:             %12lu ns  (%6.2f µs)       ║\n", 
           results.latency_p999_ns, results.latency_p999_ns / 1000.0);
    printf("║  Max:               %12lu ns  (%6.2f µs)       ║\n", 
           results.latency_max_ns, results.latency_max_ns / 1000.0);
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                 JITTER ANALYSIS                           ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Max Validation Gap: %10lu ns  (%6.2f µs)       ║\n", 
           results.max_jitter_delta_ns, results.max_jitter_delta_ns / 1000.0);
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║              OS CONTEXT SWITCH TRACKING                   ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Voluntary:         %12ld                         ║\n", results.voluntary_context_switches);
    printf("║  Involuntary:       %12ld                         ║\n", results.involuntary_context_switches);
    
    if (results.involuntary_context_switches > 0) {
        printf("║                                                           ║\n");
        printf("║  ⚠ WARNING: Involuntary switches detected!               ║\n");
        printf("║     Core pinning may not be isolated enough.             ║\n");
    } else {
        printf("║  ✓ Zero involuntary switches — clean core isolation      ║\n");
    }
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    return 0;
}
