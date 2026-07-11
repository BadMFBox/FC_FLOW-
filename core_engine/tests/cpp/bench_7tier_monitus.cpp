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
std::atomic<uint64_t> alert_pushed_count{0};
std::atomic<bool> stop_flag{false};

// Simulated 7-tier hot path
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
    
    uint64_t flow_cycle = 0;
    
    while (!stop_flag.load(std::memory_order_relaxed)) {
        input.flow_cycle = flow_cycle++;
        input.tick_salt = flow_cycle * 1000;
        
        // Fire crossbolt
        auto result = fire_crossbolt(input);
        
        if (result.has_value()) {
            // Success path
            monitus.witness_bolt_fired(sector_id, flow_cycle, result.value());
            bolt_fired_count.fetch_add(1, std::memory_order_relaxed);
            
            // Simulate 5% miss rate for alerts
            if ((flow_cycle % 20) == 0) {
                float miss_rate = 0.05f + (sector_id * 0.01f);
                uint8_t level = (miss_rate > 0.10f) ? 2 : 1;
                if (monitus.push_miss_alert(sector_id, level, miss_rate)) {
                    alert_pushed_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        } else {
            // Failure path
            monitus.witness_bolt_failed(sector_id, flow_cycle, MonitusEventType::BOLT_FAILED);
            bolt_failed_count.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

void drain_worker(Monitus& monitus) {
    while (!stop_flag.load(std::memory_order_relaxed)) {
        if (monitus.pending_alerts() > 0) {
            monitus.drain_alerts();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Final drain
    if (monitus.pending_alerts() > 0) {
        monitus.drain_alerts();
    }
}

int main() {
    if (sodium_init() < 0) {
        fprintf(stderr, "libsodium initialization failed\n");
        return 1;
    }
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║     FC_FLOW — 7-TIER MONITUS BENCH (20 seconds)          ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    Monitus monitus;
    
    // Spawn 7 sector workers (tiers)
    std::thread workers[7];
    for (uint8_t i = 0; i < 7; i++) {
        workers[i] = std::thread(tier_worker, std::ref(monitus), i);
    }
    
    // Spawn alert drain worker
    std::thread drainer(drain_worker, std::ref(monitus));
    
    printf("[Bench] 7 tiers active — running for 20 seconds...\n\n");
    
    auto start = std::chrono::steady_clock::now();
    
    // Run for 20 seconds
    std::this_thread::sleep_for(std::chrono::seconds(20));
    
    stop_flag.store(true, std::memory_order_relaxed);
    
    // Join all workers
    for (auto& w : workers) {
        w.join();
    }
    drainer.join();
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    uint64_t total_fired = bolt_fired_count.load();
    uint64_t total_failed = bolt_failed_count.load();
    uint64_t total_alerts = alert_pushed_count.load();
    uint64_t total_ops = total_fired + total_failed;
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                    BENCH RESULTS                          ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Elapsed:           %6lu ms                            ║\n", elapsed_ms);
    printf("║  Total Operations:  %12lu                         ║\n", total_ops);
    printf("║  Bolts Fired:       %12lu                         ║\n", total_fired);
    printf("║  Bolts Failed:      %12lu                         ║\n", total_failed);
    printf("║  Alerts Pushed:     %12lu                         ║\n", total_alerts);
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Throughput:        %12lu ops/sec                 ║\n", (total_ops * 1000) / elapsed_ms);
    printf("║  Per-Tier:          %12lu ops/sec                 ║\n", (total_ops * 1000) / (elapsed_ms * 7));
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
