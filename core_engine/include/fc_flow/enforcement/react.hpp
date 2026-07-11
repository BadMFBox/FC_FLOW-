#pragma once

#include "fc_flow/enforcement/blackseal.hpp"
#include "fc_flow/enforcement/reflex.hpp"
#include <atomic>
#include <cstdint>

namespace REACT {

// ═══════════════════════════════════════════════════════
// FC_FLOW — REACT
// Reactive Enforcement & Autonomous Control Time
// 
// The Orchestrator — Coordinates BlackSeal + REFLEX
// Called by Monitus on MISFIRE
// Sub-5ms total response time
// ═══════════════════════════════════════════════════════

enum class REACTStatus : uint8_t {
    CLEARED          = 0,  // all checks passed
    MISFIRE_HANDLED  = 1,  // isolation + halt completed
    ALREADY_ISOLATED = 2,  // sector was already isolated
    HALT_FAILED      = 3   // REFLEX failed to confirm halt
};

struct REACTEvent {
    uint8_t      sector_id;
    uint8_t      stack_id;
    REACTStatus  status;
    SealReason   seal_reason;      // from BlackSeal
    HaltReason   halt_reason;      // from REFLEX
    uint64_t     timestamp_ns;     // CLOCK_MONOTONIC
    uint64_t     total_latency_us; // trigger to halt complete
    bool         apex_notified;    // stub — always false for now
    
    REACTEvent()
        : sector_id(0)
        , stack_id(0)
        , status(REACTStatus::CLEARED)
        , seal_reason(SealReason::MISFIRE_PIN_A)
        , halt_reason(HaltReason::CROSSBOLT_MISFIRE)
        , timestamp_ns(0)
        , total_latency_us(0)
        , apex_notified(false)
    {}
};

class REACTOrchestrator {
public:
    REACTOrchestrator(BlackSeal& seal, REFLEX& reflex);

    // Called by Monitus on MISFIRE
    REACTEvent handle_misfire(uint8_t sector_id,
                              uint8_t stack_id,
                              SealReason reason) noexcept;

    // Called by Monitus on BOLT LOCKED — sector cleared
    REACTEvent handle_cleared(uint8_t sector_id,
                              uint8_t stack_id) noexcept;

    bool is_sector_safe(uint8_t sector_id) const noexcept;
    
    uint64_t get_total_events() const noexcept { return total_events_.load(std::memory_order_acquire); }
    uint64_t get_total_misfires() const noexcept { return total_misfires_.load(std::memory_order_acquire); }

private:
    BlackSeal& seal_;
    REFLEX&    reflex_;
    std::atomic<uint64_t> total_events_{0};
    std::atomic<uint64_t> total_misfires_{0};
};

} // namespace REACT
