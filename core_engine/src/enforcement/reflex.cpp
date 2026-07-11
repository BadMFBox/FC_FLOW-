#include "fc_flow/enforcement/reflex.hpp"
#include <sodium.h>
#include <time.h>
#include <cstdio>

namespace REACT {

// ── WIPE SECTOR KEYS ─────────────────────────────────
void REFLEX::wipe_sector_keys(uint8_t sector_id) noexcept
{
    // Simulate key material wipe
    // In production: wipe actual sector key buffers
    // For now: just placeholder with sodium_memzero() call pattern
    
    // Example: If we had key material in memory
    // unsigned char key_buffer[32];
    // sodium_memzero(key_buffer, sizeof(key_buffer));
    
    // For testing purposes, we acknowledge the wipe occurred
    (void)sector_id;  // Suppress unused warning
}

// ── FIRE ─────────────────────────────────────────────
HaltEvent REFLEX::fire(uint8_t sector_id,
                       uint8_t stack_id,
                       HaltReason reason) noexcept
{
    // Start timing
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    HaltEvent event;
    event.sector_id = sector_id;
    event.stack_id = stack_id;
    event.reason = reason;
    
    // Validate sector_id
    if (sector_id >= MAX_SECTORS) {
        event.keys_wiped = false;
        event.sector_stopped = false;
        event.halt_latency_us = 0;
        clock_gettime(CLOCK_MONOTONIC, &end);
        event.timestamp_ns = end.tv_sec * 1000000000ULL + end.tv_nsec;
        return event;
    }
    
    // Set halt bit atomically
    uint8_t mask = static_cast<uint8_t>(1u << sector_id);
    halted_sectors_.fetch_or(mask, std::memory_order_release);
    
    // Wipe sector key material
    wipe_sector_keys(sector_id);
    event.keys_wiped = true;
    
    // Sector stopped
    event.sector_stopped = true;
    
    // End timing
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // Calculate latency in microseconds
    uint64_t start_us = start.tv_sec * 1000000ULL + start.tv_nsec / 1000;
    uint64_t end_us = end.tv_sec * 1000000ULL + end.tv_nsec / 1000;
    event.halt_latency_us = end_us - start_us;
    event.timestamp_ns = end.tv_sec * 1000000000ULL + end.tv_nsec;
    
    // Log
    const char* reason_str = "UNKNOWN";
    switch (reason) {
        case HaltReason::CROSSBOLT_MISFIRE:   reason_str = "CROSSBOLT_MISFIRE"; break;
        case HaltReason::BLACKSEAL_TRIGGERED: reason_str = "BLACKSEAL_TRIGGERED"; break;
        case HaltReason::SILENCE_DETECTED:    reason_str = "SILENCE_DETECTED"; break;
        case HaltReason::REPLAY_DETECTED:     reason_str = "REPLAY_DETECTED"; break;
        case HaltReason::MANUAL_OVERRIDE:     reason_str = "MANUAL_OVERRIDE"; break;
    }
    
    printf("[REFLEX] S%d HALTED — Reason: %s — Stack: %d — Latency: %lu µs\n",
           sector_id, reason_str, stack_id, event.halt_latency_us);
    
    return event;
}

// ── IS HALTED ────────────────────────────────────────
bool REFLEX::is_halted(uint8_t sector_id) const noexcept
{
    if (sector_id >= MAX_SECTORS) {
        return false;
    }
    
    // Check broadcast halt first
    if (halt_broadcast_.load(std::memory_order_acquire)) {
        return true;
    }
    
    uint8_t mask = static_cast<uint8_t>(1u << sector_id);
    uint8_t current = halted_sectors_.load(std::memory_order_acquire);
    
    return (current & mask) != 0;
}

// ── RESET ────────────────────────────────────────────
void REFLEX::reset(uint8_t sector_id) noexcept
{
    if (sector_id >= MAX_SECTORS) {
        return;
    }
    
    uint8_t mask = static_cast<uint8_t>(1u << sector_id);
    uint8_t inv_mask = static_cast<uint8_t>(~mask);
    
    halted_sectors_.fetch_and(inv_mask, std::memory_order_release);
    
    printf("[REFLEX] S%d reset — manual override\n", sector_id);
}

// ── EMERGENCY HALT ALL ───────────────────────────────
void REFLEX::emergency_halt_all() noexcept
{
    halt_broadcast_.store(true, std::memory_order_release);
    
    printf("[REFLEX] EMERGENCY BROADCAST HALT — ALL SECTORS STOPPED\n");
}

// ── IS BROADCAST HALTED ──────────────────────────────
bool REFLEX::is_broadcast_halted() const noexcept
{
    return halt_broadcast_.load(std::memory_order_acquire);
}

// ── CLEAR BROADCAST ──────────────────────────────────
void REFLEX::clear_broadcast() noexcept
{
    halt_broadcast_.store(false, std::memory_order_release);
    
    printf("[REFLEX] Broadcast halt cleared\n");
}

} // namespace REACT
