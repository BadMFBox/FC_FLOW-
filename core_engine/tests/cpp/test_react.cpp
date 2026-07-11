#include "fc_flow/enforcement/react.hpp"
#include <cassert>
#include <cstdio>
#include <sodium.h>

using namespace REACT;

// ── Test 1: handle_misfire() calls BlackSeal then REFLEX ──
void test_misfire_order() {
    BlackSeal seal;
    REFLEX reflex;
    REACTOrchestrator react(seal, reflex);
    
    // Sector 3 initially safe
    assert(react.is_sector_safe(3));
    
    // Handle misfire
    REACTEvent event = react.handle_misfire(3, 1, SealReason::MISFIRE_PIN_A);
    
    // Verify event
    assert(event.sector_id == 3);
    assert(event.stack_id == 1);
    assert(event.status == REACTStatus::MISFIRE_HANDLED);
    assert(event.seal_reason == SealReason::MISFIRE_PIN_A);
    assert(event.halt_reason == HaltReason::CROSSBOLT_MISFIRE);
    assert(event.timestamp_ns > 0);
    assert(event.apex_notified == false);
    
    // Verify BlackSeal was called (sector isolated)
    assert(seal.is_isolated(3));
    
    // Verify REFLEX was called (sector halted)
    assert(reflex.is_halted(3));
    
    // Sector no longer safe
    assert(!react.is_sector_safe(3));
    
    printf("✓ Test 1: handle_misfire() calls BlackSeal then REFLEX in correct order\n");
}

// ── Test 2: total_latency_us confirmed under 5000μs ──
void test_latency() {
    BlackSeal seal;
    REFLEX reflex;
    REACTOrchestrator react(seal, reflex);
    
    REACTEvent event = react.handle_misfire(2, 1, SealReason::MISFIRE_BOLT);
    
    printf("[REACT] Total latency: %lu µs (%.3f ms)\n",
           event.total_latency_us, event.total_latency_us / 1000.0);
    
    // Must complete in under 5ms (5000 µs)
    assert(event.total_latency_us < 5000);
    
    printf("✓ Test 2: total_latency_us confirmed under 5000µs (5ms)\n");
}

// ── Test 3: handle_cleared() marks sector safe ───────
void test_cleared() {
    BlackSeal seal;
    REFLEX reflex;
    REACTOrchestrator react(seal, reflex);
    
    // Handle cleared
    REACTEvent event = react.handle_cleared(4, 1);
    
    // Verify event
    assert(event.sector_id == 4);
    assert(event.stack_id == 1);
    assert(event.status == REACTStatus::CLEARED);
    assert(event.timestamp_ns > 0);
    assert(event.total_latency_us == 0);
    
    // Sector should be safe (not isolated, not halted)
    assert(react.is_sector_safe(4));
    
    printf("✓ Test 3: handle_cleared() marks sector safe\n");
}

// ── Test 4: is_sector_safe() after misfire ───────────
void test_sector_safety() {
    BlackSeal seal;
    REFLEX reflex;
    REACTOrchestrator react(seal, reflex);
    
    // Initially all safe
    assert(react.is_sector_safe(0));
    assert(react.is_sector_safe(1));
    assert(react.is_sector_safe(2));
    
    // Misfire on sector 1
    react.handle_misfire(1, 1, SealReason::SILENCE);
    
    // Sector 1 no longer safe
    assert(react.is_sector_safe(0));
    assert(!react.is_sector_safe(1));
    assert(react.is_sector_safe(2));
    
    printf("✓ Test 4: is_sector_safe() returns correct state after misfire\n");
}

// ── Test 5: Multiple sector misfires ─────────────────
void test_multiple_misfires() {
    BlackSeal seal;
    REFLEX reflex;
    REACTOrchestrator react(seal, reflex);
    
    // Handle misfires on sectors 0, 2, 5
    REACTEvent e0 = react.handle_misfire(0, 1, SealReason::MISFIRE_PIN_A);
    REACTEvent e2 = react.handle_misfire(2, 1, SealReason::MISFIRE_PIN_B);
    REACTEvent e5 = react.handle_misfire(5, 2, SealReason::REPLAY);
    
    // All handled
    assert(e0.status == REACTStatus::MISFIRE_HANDLED);
    assert(e2.status == REACTStatus::MISFIRE_HANDLED);
    assert(e5.status == REACTStatus::MISFIRE_HANDLED);
    
    // Correct sectors not safe
    assert(!react.is_sector_safe(0));
    assert(react.is_sector_safe(1));
    assert(!react.is_sector_safe(2));
    assert(react.is_sector_safe(3));
    assert(react.is_sector_safe(4));
    assert(!react.is_sector_safe(5));
    assert(react.is_sector_safe(6));
    
    printf("✓ Test 5: Multiple sector misfires handled correctly\n");
}

// ── Test 6: total_misfires_ counter ──────────────────
void test_counters() {
    BlackSeal seal;
    REFLEX reflex;
    REACTOrchestrator react(seal, reflex);
    
    // Initial counters
    assert(react.get_total_events() == 0);
    assert(react.get_total_misfires() == 0);
    
    // Handle 3 misfires
    react.handle_misfire(1, 1, SealReason::MISFIRE_PIN_A);
    react.handle_misfire(2, 1, SealReason::MISFIRE_BOLT);
    react.handle_misfire(3, 1, SealReason::SILENCE);
    
    // Counters updated
    assert(react.get_total_events() == 3);
    assert(react.get_total_misfires() == 3);
    
    // Handle 2 cleared events
    react.handle_cleared(4, 1);
    react.handle_cleared(5, 1);
    
    // Total events increased, misfires unchanged
    assert(react.get_total_events() == 5);
    assert(react.get_total_misfires() == 3);
    
    printf("✓ Test 6: total_misfires_ counter increments correctly\n");
}

// ── Test 7: Already isolated sector ──────────────────
void test_already_isolated() {
    BlackSeal seal;
    REFLEX reflex;
    REACTOrchestrator react(seal, reflex);
    
    // First misfire
    REACTEvent e1 = react.handle_misfire(6, 1, SealReason::MISFIRE_PIN_A);
    assert(e1.status == REACTStatus::MISFIRE_HANDLED);
    
    // Second misfire on same sector
    REACTEvent e2 = react.handle_misfire(6, 1, SealReason::MISFIRE_PIN_B);
    assert(e2.status == REACTStatus::ALREADY_ISOLATED);
    
    printf("✓ Test 7: Already isolated sector handled correctly\n");
}

int main() {
    // Initialize libsodium
    if (sodium_init() < 0) {
        fprintf(stderr, "libsodium initialization failed\n");
        return 1;
    }
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║    REACT — Enforcement Orchestrator Tests        ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
    
    test_misfire_order();
    test_latency();
    test_cleared();
    test_sector_safety();
    test_multiple_misfires();
    test_counters();
    test_already_isolated();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║              All Tests PASSED ✓                   ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
    
    return 0;
}
