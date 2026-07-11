#include "fc_flow/enforcement/blackseal.hpp"
#include <chrono>
#include <cstdio>

namespace REACT {

// ── ISOLATE ──────────────────────────────────────────
SealEvent BlackSeal::isolate(uint8_t sector_id, 
                             uint8_t stack_id,
                             SealReason reason) noexcept
{
    // Validate sector_id
    if (sector_id >= MAX_SECTORS) {
        SealEvent failed;
        failed.sector_id = sector_id;
        failed.stack_id = stack_id;
        failed.reason = reason;
        failed.timestamp_ns = std::chrono::high_resolution_clock::now()
                                .time_since_epoch()
                                .count();
        failed.isolated = false;
        return failed;
    }
    
    // Set bit atomically
    uint8_t mask = static_cast<uint8_t>(1u << sector_id);
    uint8_t old_val = isolated_sectors_.fetch_or(mask, std::memory_order_release);
    
    // Create event
    SealEvent event;
    event.sector_id = sector_id;
    event.stack_id = stack_id;
    event.reason = reason;
    event.timestamp_ns = std::chrono::high_resolution_clock::now()
                           .time_since_epoch()
                           .count();
    event.isolated = true;
    
    // Log
    const char* reason_str = "UNKNOWN";
    switch (reason) {
        case SealReason::MISFIRE_PIN_A: reason_str = "MISFIRE_PIN_A"; break;
        case SealReason::MISFIRE_PIN_B: reason_str = "MISFIRE_PIN_B"; break;
        case SealReason::MISFIRE_BOLT:  reason_str = "MISFIRE_BOLT";  break;
        case SealReason::SILENCE:       reason_str = "SILENCE";       break;
        case SealReason::REPLAY:        reason_str = "REPLAY";        break;
    }
    
    bool was_already_isolated = (old_val & mask) != 0;
    
    printf("[BlackSeal] S%d isolated — Reason: %s — Stack: %d — Timestamp: %lu ns\n",
           sector_id, reason_str, stack_id, event.timestamp_ns);
    
    if (was_already_isolated) {
        printf("[BlackSeal] WARNING — S%d was already isolated\n", sector_id);
    }
    
    return event;
}

// ── IS ISOLATED ──────────────────────────────────────
bool BlackSeal::is_isolated(uint8_t sector_id) const noexcept
{
    if (sector_id >= MAX_SECTORS) {
        return false;
    }
    
    uint8_t mask = static_cast<uint8_t>(1u << sector_id);
    uint8_t current = isolated_sectors_.load(std::memory_order_acquire);
    
    return (current & mask) != 0;
}

// ── RELEASE ──────────────────────────────────────────
void BlackSeal::release(uint8_t sector_id) noexcept
{
    if (sector_id >= MAX_SECTORS) {
        return;
    }
    
    uint8_t mask = static_cast<uint8_t>(1u << sector_id);
    uint8_t inv_mask = static_cast<uint8_t>(~mask);
    
    isolated_sectors_.fetch_and(inv_mask, std::memory_order_release);
    
    printf("[BlackSeal] S%d released — manual override\n", sector_id);
}

} // namespace REACT
