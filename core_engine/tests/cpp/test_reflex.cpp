#include "fc_flow/enforcement/reflex.hpp"
#include <cassert>
#include <cstdio>
#include <sodium.h>

using namespace REACT;

// ── Test 1: fire() halts sector and sets bit ────────
void test_fire_basic() {
    REFLEX reflex;
    
    // Initially not halted
    assert(!reflex.is_halted(2));
    
    // Fire halt on sector 2
    HaltEvent event = reflex.fire(2, 1, HaltReason::CROSSBOLT_MISFIRE);
    
    // Verify event
    assert(event.sector_id == 2);
    assert(event.stack_id == 1);
    assert(event.reason == HaltReason::CROSSBOLT_MISFIRE);
    assert(event.keys_wiped == true);
    assert(event.sector_stopped == true);
    assert(event.timestamp_ns > 0);
    
    // Verify halted state
    assert(reflex.is_halted(2));
    
    printf("✓ Test 1: fire() halts sector and sets bit\n");
}

// ── Test 2: halt_latency_us confirmed under 5000μs ──
void test_latency() {
    REFLEX reflex;
    
    HaltEvent event = reflex.fire(3, 1, HaltReason::BLACKSEAL_TRIGGERED);
    
    printf("[REFLEX] Measured latency: %lu µs (%.3f ms)\n", 
           event.halt_latency_us, event.halt_latency_us / 1000.0);
    
    // Must complete in under 5ms (5000 µs)
    assert(event.halt_latency_us < 5000);
    
    printf("✓ Test 2: halt_latency_us confirmed under 5000µs (5ms)\n");
}

// ── Test 3: halt_broadcast_ halts all 7 sectors ─────
void test_broadcast_halt() {
    REFLEX reflex;
    
    // Initially no sectors halted
    for (uint8_t i = 0; i < 7; i++) {
        assert(!reflex.is_halted(i));
    }
    
    // Emergency halt all
    reflex.emergency_halt_all();
    
    // Verify broadcast flag
    assert(reflex.is_broadcast_halted());
    
    // All sectors should report as halted
    for (uint8_t i = 0; i < 7; i++) {
        assert(reflex.is_halted(i));
    }
    
    // Clear broadcast
    reflex.clear_broadcast();
    assert(!reflex.is_broadcast_halted());
    
    // Individual sectors should no longer be halted via broadcast
    for (uint8_t i = 0; i < 7; i++) {
        assert(!reflex.is_halted(i));
    }
    
    printf("✓ Test 3: halt_broadcast_ halts all 7 sectors simultaneously\n");
}

// ── Test 4: is_halted() returns correct state ───────
void test_is_halted() {
    REFLEX reflex;
    
    // Halt sectors 0, 3, 5
    reflex.fire(0, 1, HaltReason::SILENCE_DETECTED);
    reflex.fire(3, 1, HaltReason::REPLAY_DETECTED);
    reflex.fire(5, 2, HaltReason::CROSSBOLT_MISFIRE);
    
    // Verify correct state
    assert(reflex.is_halted(0));
    assert(!reflex.is_halted(1));
    assert(!reflex.is_halted(2));
    assert(reflex.is_halted(3));
    assert(!reflex.is_halted(4));
    assert(reflex.is_halted(5));
    assert(!reflex.is_halted(6));
    
    printf("✓ Test 4: is_halted() returns correct state\n");
}

// ── Test 5: reset() clears bit — manual override ────
void test_reset() {
    REFLEX reflex;
    
    // Halt sector 4
    reflex.fire(4, 1, HaltReason::MANUAL_OVERRIDE);
    assert(reflex.is_halted(4));
    
    // Reset sector 4
    reflex.reset(4);
    assert(!reflex.is_halted(4));
    
    printf("✓ Test 5: reset() clears bit — manual override only\n");
}

// ── Test 6: Invalid sector ID ───────────────────────
void test_invalid_sector() {
    REFLEX reflex;
    
    // Try to halt invalid sector (>= 7)
    HaltEvent event = reflex.fire(10, 1, HaltReason::CROSSBOLT_MISFIRE);
    
    // Should fail gracefully
    assert(event.keys_wiped == false);
    assert(event.sector_stopped == false);
    assert(!reflex.is_halted(10));
    
    printf("✓ Test 6: Invalid sector ID handled\n");
}

// ── Test 7: Multiple sectors + broadcast interaction ─
void test_mixed_halt() {
    REFLEX reflex;
    
    // Halt sector 1 individually
    reflex.fire(1, 1, HaltReason::CROSSBOLT_MISFIRE);
    assert(reflex.is_halted(1));
    
    // Broadcast halt
    reflex.emergency_halt_all();
    
    // Sector 1 still halted (both individual + broadcast)
    assert(reflex.is_halted(1));
    
    // Clear broadcast
    reflex.clear_broadcast();
    
    // Sector 1 still halted (individual bit still set)
    assert(reflex.is_halted(1));
    
    // Other sectors no longer halted
    assert(!reflex.is_halted(0));
    assert(!reflex.is_halted(2));
    
    printf("✓ Test 7: Mixed individual + broadcast halt interaction\n");
}

int main() {
    // Initialize libsodium
    if (sodium_init() < 0) {
        fprintf(stderr, "libsodium initialization failed\n");
        return 1;
    }
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║       REFLEX — Autonomous Surgical Halt Tests    ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
    
    test_fire_basic();
    test_latency();
    test_broadcast_halt();
    test_is_halted();
    test_reset();
    test_invalid_sector();
    test_mixed_halt();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║              All Tests PASSED ✓                   ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
    
    return 0;
}
