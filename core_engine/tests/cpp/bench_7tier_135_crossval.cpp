#include "fc_flow/enforcement/monitus.hpp"
#include "fc_flow/crypto/crossbolt_lock.hpp"
#include "fc_flow/crypto/crossbolt.hpp"
#include <cassert>
#include <cstdio>
#include <sodium.h>
#include <chrono>
#include <thread>
#include <atomic>

using namespace fc_flow::enforcement;
using namespace fc_flow::crypto;

std::atomic<uint64_t> bolt_fired_count{0};
std::atomic<uint64_t> bolt_failed_count{0};
std::atomic<uint64_t> crossval_performed{0};
std::atomic<bool> stop_flag{false};

// 1-3-5 cross-validation pattern
bool should_crossval(uint64_t loop_count) {
    uint64_t cycle = loop_count % 5;
    return (cycle == 0 || cycle == 2 || cycle == 4);  // 1st, 3rd, 5th (0-indexed)
}

// Simulated 7-tier hot path with 1-3-5 cross-validation
void tier_worker(Monitus& monitus, uint8_t sector_id) {
    BoltInput input;
    sodium_memzero(&input, sizeof(input));
    input.sector_id = sector_id;
    input.stack_id = 0;
    input.flow_cycle = 0;
    
    // Generate some entropy
    randombytes_buf(input.master_secret.data(), 32);
    randombytes_buf(input.hardware_dna.data(), 64);
    randombytes_buf(input.entropy.data(), 32);
    
    uint64_t loop_count = 0;
    
    while (!stop_flag.load(std::memory_order_relaxed)) {
        input.flow_cycle = loop_count;
        input.tick_salt = loop_count * 1000;
        
        // Fire crossbolt
        auto fire_result = fire_crossbolt(input);
        
        if (fire_result.has_value()) {
            CrossBolt& bolt = fire_result.value();
            
            // Cross-validation on 1-3-5 pattern
            if (should_crossval(loop_count)) {
                auto verify_result = verify_crossbolt(bolt, input);
                crossval_performed.fetch_add(1, std::memory_order_relaxed);
                
                if (verify_result.has_value()) {
                    // Success path
                    monitus.witness_bolt_fired(sector_id, loop_count, bolt);
                    bolt_fired_count.fetch_add(1, std::memory_order_relaxed);
                } else {
                    // Verification failed
                    monitus.witness_bolt_failed(sector_id, loop_count, MonitusEventType::BOLT_FAILED);
                    bolt_failed_count.fetch_add(1, std::memory_order_relaxed);
                }
            } else {
                // No cross-validation this loop — trust and witness
                monitus.witness_bolt_fired(sector_id, loop_count, bolt);
                bolt_fired_count.fetch_add(1, std::memory_order_relaxed);
            }
            
            bolt.wipe();
        } else {
            // Fire failed
            monitus.witness_bolt_failed(sector_id, loop_count, MonitusEventType::BOLT_FAILED);
            bolt_failed_count.fetch_add(1, std::memory_order_relaxed);
        }
        
        loop_count++;
    }
}

int main() {
    if (sodium_init() < 0) {
        fprintf(stderr, "libsodium initialization failed\n");
        return 1;
    }
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║   FC_FLOW — 7-TIER 1-3-5 CROSSVAL BENCH (30 seconds)     ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    Monitus monitus;
    
    // Spawn 7 sector workers (tiers)
    std::thread workers[7];
    for (uint8_t i = 0; i < 7; i++) {
        workers[i] = std::thread(tier_worker, std::ref(monitus), i);
    }
    
    printf("[Bench] 7 tiers active with 1-3-5 cross-validation pattern\n");
    printf("[Bench] Running for 30 seconds...\n\n");
    
    auto start = std::chrono::steady_clock::now();
    
    // Run for 30 seconds
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    stop_flag.store(true, std::memory_order_relaxed);
    
    // Join all workers
    for (auto& w : workers) {
        w.join();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    uint64_t total_fired = bolt_fired_count.load();
    uint64_t total_failed = bolt_failed_count.load();
    uint64_t total_crossval = crossval_performed.load();
    uint64_t total_ops = total_fired + total_failed;
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                    BENCH RESULTS                          ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Elapsed:           %6lu ms                            ║\n", elapsed_ms);
    printf("║  Total Operations:  %12lu                         ║\n", total_ops);
    printf("║  Bolts Fired:       %12lu                         ║\n", total_fired);
    printf("║  Bolts Failed:      %12lu                         ║\n", total_failed);
    printf("║  Cross-Validations: %12lu (60%% expected)         ║\n", total_crossval);
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Throughput:        %12lu ops/sec                 ║\n", (total_ops * 1000) / elapsed_ms);
    printf("║  Per-Tier:          %12lu ops/sec                 ║\n", (total_ops * 1000) / (elapsed_ms * 7));
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    
    // Calculate actual cross-validation percentage
    float crossval_pct = (total_crossval * 100.0f) / total_ops;
    printf("║  Actual CrossVal:   %11.1f%%                       ║\n", crossval_pct);
    
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    // Verify event counts
    uint64_t witnessed_fired = monitus.get_event_count(MonitusEventType::BOLT_FIRED);
    uint64_t witnessed_failed = monitus.get_event_count(MonitusEventType::BOLT_FAILED);
    
    printf("[Verification]\n");
    printf("  Witnessed fired:  %lu\n", witnessed_fired);
    printf("  Witnessed failed: %lu\n", witnessed_failed);
    printf("  Match: %s\n\n", (witnessed_fired == total_fired && witnessed_failed == total_failed) ? "✓" : "✗");
    
    return 0;
}
