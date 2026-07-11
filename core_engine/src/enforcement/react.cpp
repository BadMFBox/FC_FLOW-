#include "fc_flow/enforcement/react.hpp"
#include <time.h>
#include <cstdio>

namespace REACT {

// ── CONSTRUCTOR ──────────────────────────────────────
REACTOrchestrator::REACTOrchestrator(BlackSeal& seal, REFLEX& reflex)
    : seal_(seal)
    , reflex_(reflex)
    , total_events_(0)
    , total_misfires_(0)
{}

// ── HANDLE MISFIRE ───────────────────────────────────
REACTEvent REACTOrchestrator::handle_misfire(uint8_t sector_id,
                                             uint8_t stack_id,
                                             SealReason reason) noexcept
{
    // Start timing
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    REACTEvent event;
    event.sector_id = sector_id;
    event.stack_id = stack_id;
    event.seal_reason = reason;
    
    // Increment counters
    total_events_.fetch_add(1, std::memory_order_relaxed);
    total_misfires_.fetch_add(1, std::memory_order_relaxed);
    
    // Check if already isolated
    if (seal_.is_isolated(sector_id)) {
        event.status = REACTStatus::ALREADY_ISOLATED;
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        event.timestamp_ns = end.tv_sec * 1000000000ULL + end.tv_nsec;
        
        uint64_t start_us = start.tv_sec * 1000000ULL + start.tv_nsec / 1000;
        uint64_t end_us = end.tv_sec * 1000000ULL + end.tv_nsec / 1000;
        event.total_latency_us = end_us - start_us;
        
        printf("[REACT] S%d MISFIRE — already isolated\n", sector_id);
        return event;
    }
    
    // Step 1: Call BlackSeal.isolate() — sector isolated
    SealEvent seal_event = seal_.isolate(sector_id, stack_id, reason);
    
    // Step 2: Call REFLEX.fire() — sector halted, keys wiped
    HaltReason halt_reason;
    switch (reason) {
        case SealReason::MISFIRE_PIN_A: halt_reason = HaltReason::CROSSBOLT_MISFIRE; break;
        case SealReason::MISFIRE_PIN_B: halt_reason = HaltReason::CROSSBOLT_MISFIRE; break;
        case SealReason::MISFIRE_BOLT:  halt_reason = HaltReason::CROSSBOLT_MISFIRE; break;
        case SealReason::SILENCE:       halt_reason = HaltReason::SILENCE_DETECTED; break;
        case SealReason::REPLAY:        halt_reason = HaltReason::REPLAY_DETECTED; break;
        default:                        halt_reason = HaltReason::CROSSBOLT_MISFIRE; break;
    }
    
    HaltEvent halt_event = reflex_.fire(sector_id, stack_id, halt_reason);
    event.halt_reason = halt_reason;
    
    // Step 3: Verify halt completed
    if (!halt_event.sector_stopped) {
        event.status = REACTStatus::HALT_FAILED;
    } else {
        event.status = REACTStatus::MISFIRE_HANDLED;
    }
    
    // Step 4: Log full REACTEvent — timestamped record
    clock_gettime(CLOCK_MONOTONIC, &end);
    event.timestamp_ns = end.tv_sec * 1000000000ULL + end.tv_nsec;
    
    uint64_t start_us = start.tv_sec * 1000000ULL + start.tv_nsec / 1000;
    uint64_t end_us = end.tv_sec * 1000000ULL + end.tv_nsec / 1000;
    event.total_latency_us = end_us - start_us;
    
    // Step 5: Signal ApexPredator — stub for now
    event.apex_notified = false;  // ApexPredator built next
    
    printf("[REACT] S%d MISFIRE HANDLED — Total latency: %lu µs\n",
           sector_id, event.total_latency_us);
    
    return event;
}

// ── HANDLE CLEARED ───────────────────────────────────
REACTEvent REACTOrchestrator::handle_cleared(uint8_t sector_id,
                                             uint8_t stack_id) noexcept
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    REACTEvent event;
    event.sector_id = sector_id;
    event.stack_id = stack_id;
    event.status = REACTStatus::CLEARED;
    event.timestamp_ns = now.tv_sec * 1000000000ULL + now.tv_nsec;
    event.total_latency_us = 0;
    event.apex_notified = false;
    
    // Increment event counter
    total_events_.fetch_add(1, std::memory_order_relaxed);
    
    printf("[REACT] S%d CLEARED — bolt locked successfully\n", sector_id);
    
    return event;
}

// ── IS SECTOR SAFE ───────────────────────────────────
bool REACTOrchestrator::is_sector_safe(uint8_t sector_id) const noexcept
{
    // Sector is safe if NOT isolated and NOT halted
    return !seal_.is_isolated(sector_id) && !reflex_.is_halted(sector_id);
}

} // namespace REACT
