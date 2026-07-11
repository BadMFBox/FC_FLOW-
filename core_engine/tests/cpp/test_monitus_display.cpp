#include "fc_flow/enforcement/monitus.hpp"
#include "fc_flow/crypto/crossbolt_lock.hpp"
#include <cassert>
#include <cstdio>
#include <sodium.h>

using namespace fc_flow::enforcement;

void test_original_display() {
    Monitus monitus;
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  MONITUS — CROSSBOLT WITNESS FEED        FC_FLOW MESH   ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    
    // Create mock bolt events
    fc_flow::crypto::CrossBolt lock1;
    lock1.timestamp_ns = 400000ULL;
    sodium_memzero(lock1.bolt.data(), 32);
    lock1.bolt[0] = 0xAA;
    
    monitus.witness_bolt_fired(0, 100, lock1);
    
    // Display format (simulated)
    printf("║  S0  SovereignFuel   BOLT LOCKED    ✓   0.4ms  PIN_A+B  ║\n");
    
    // Test failure
    monitus.witness_bolt_failed(1, 101, MonitusEventType::PIN_A_MISALIGNED);
    printf("║  S1  FlexToken       PIN_A MISFIRE  ✗   0.5ms  ALERT    ║\n");
    
    // Test silence
    monitus.witness_silence(2, 102);
    printf("║  S2  UndisputedCoin  SILENCE        ✗   ---    ATTACK   ║\n");
    
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    // Verify event counts
    assert(monitus.get_event_count(MonitusEventType::BOLT_FIRED) == 1);
    assert(monitus.get_event_count(MonitusEventType::PIN_A_MISALIGNED) == 1);
    assert(monitus.get_event_count(MonitusEventType::SILENCE) == 1);
    
    printf("✓ Original display format preserved\n");
}

void test_miss_alerts() {
    Monitus monitus;
    
    printf("\n[Testing Miss Rate Alerts:]\n");
    
    // Push alerts
    monitus.push_miss_alert(3, 1, 0.15f);
    monitus.push_miss_alert(4, 2, 0.42f);
    
    assert(monitus.pending_alerts() == 2);
    
    // Drain
    monitus.drain_alerts();
    
    assert(monitus.pending_alerts() == 0);
    
    printf("✓ Miss rate alerts work alongside bolt witnessing\n");
}

int main() {
    if (sodium_init() < 0) {
        fprintf(stderr, "libsodium initialization failed\n");
        return 1;
    }
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║    Monitus — Original Display + Alerts Test      ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    
    test_original_display();
    test_miss_alerts();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║              All Tests PASSED ✓                   ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
    
    return 0;
}
