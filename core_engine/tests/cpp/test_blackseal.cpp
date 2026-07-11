#include "fc_flow/enforcement/blackseal.hpp"
#include <cassert>
#include <cstdio>
#include <chrono>
#include <thread>

using namespace REACT;

// ── Test 1: Isolate fires and sets bit ──────────────
void test_isolate_basic() {
    BlackSeal seal;
    
    // Initially not isolated
    assert(!seal.is_isolated(3));
    
    // Isolate sector 3
    SealEvent event = seal.isolate(3, 1, SealReason::MISFIRE_PIN_A);
    
    // Verify event
    assert(event.sector_id == 3);
    assert(event.stack_id == 1);
    assert(event.reason == SealReason::MISFIRE_PIN_A);
    assert(event.isolated == true);
    assert(event.timestamp_ns > 0);
    
    // Verify isolation
    assert(seal.is_isolated(3));
    
    printf("✓ Test 1: Isolate fires and sets bit\n");
}

// ── Test 2: Multiple sectors isolated ───────────────
void test_multiple_sectors() {
    BlackSeal seal;
    
    // Isolate sectors 0, 2, 5
    seal.isolate(0, 1, SealReason::MISFIRE_PIN_A);
    seal.isolate(2, 1, SealReason::MISFIRE_BOLT);
    seal.isolate(5, 2, SealReason::SILENCE);
    
    // Verify
    assert(seal.is_isolated(0));
    assert(!seal.is_isolated(1));
    assert(seal.is_isolated(2));
    assert(!seal.is_isolated(3));
    assert(!seal.is_isolated(4));
    assert(seal.is_isolated(5));
    assert(!seal.is_isolated(6));
    
    printf("✓ Test 2: Multiple sectors isolated correctly\n");
}

// ── Test 3: Release clears bit ──────────────────────
void test_release() {
    BlackSeal seal;
    
    // Isolate sector 4
    seal.isolate(4, 1, SealReason::REPLAY);
    assert(seal.is_isolated(4));
    
    // Release sector 4
    seal.release(4);
    assert(!seal.is_isolated(4));
    
    printf("✓ Test 3: Release clears bit\n");
}

// ── Test 4: Sub-2ms performance ─────────────────────
void test_performance() {
    BlackSeal seal;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    SealEvent event = seal.isolate(2, 1, SealReason::MISFIRE_PIN_B);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    auto duration_us = duration_ns / 1000.0;
    
    printf("[BlackSeal] isolate() took %.2f µs (%.6f ms)\n", 
           duration_us, duration_us / 1000.0);
    
    // Must complete in under 2ms (2,000,000 ns)
    assert(duration_ns < 2000000);
    
    printf("✓ Test 4: Sub-2ms performance — %.2f µs\n", duration_us);
}

// ── Test 5: Invalid sector ID ───────────────────────
void test_invalid_sector() {
    BlackSeal seal;
    
    // Try to isolate invalid sector (>= 7)
    SealEvent event = seal.isolate(10, 1, SealReason::MISFIRE_PIN_A);
    
    // Should fail gracefully
    assert(event.isolated == false);
    assert(!seal.is_isolated(10));
    
    printf("✓ Test 5: Invalid sector ID handled\n");
}

// ── Test 6: Already isolated warning ────────────────
void test_double_isolate() {
    BlackSeal seal;
    
    // Isolate sector 1
    seal.isolate(1, 1, SealReason::MISFIRE_PIN_A);
    assert(seal.is_isolated(1));
    
    // Isolate again
    SealEvent event2 = seal.isolate(1, 1, SealReason::MISFIRE_PIN_B);
    
    // Should still succeed but log warning
    assert(event2.isolated == true);
    assert(seal.is_isolated(1));
    
    printf("✓ Test 6: Double isolate handled with warning\n");
}

int main() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║        BlackSeal — Sector Isolation Tests        ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
    
    test_isolate_basic();
    test_multiple_sectors();
    test_release();
    test_performance();
    test_invalid_sector();
    test_double_isolate();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║              All Tests PASSED ✓                   ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
    
    return 0;
}
