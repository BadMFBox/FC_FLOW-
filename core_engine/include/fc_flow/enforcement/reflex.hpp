#pragma once

#include <atomic>
#include <cstdint>

namespace REACT {

// ═══════════════════════════════════════════════════════
// FC_FLOW — REFLEX
// Autonomous Surgical Halt
// 
// Called by REACT when BlackSeal isolates a sector
// Sub-5ms guaranteed — autonomous kill-chain
// ═══════════════════════════════════════════════════════

enum class HaltReason : uint8_t {
    CROSSBOLT_MISFIRE   = 0,
    BLACKSEAL_TRIGGERED = 1,
    SILENCE_DETECTED    = 2,
    REPLAY_DETECTED     = 3,
    MANUAL_OVERRIDE     = 4
};

struct HaltEvent {
    uint8_t    sector_id;
    uint8_t    stack_id;
    HaltReason reason;
    uint64_t   timestamp_ns;
    uint64_t   halt_latency_us;  // time from trigger to halt
    bool       keys_wiped;
    bool       sector_stopped;
    
    HaltEvent()
        : sector_id(0)
        , stack_id(0)
        , reason(HaltReason::CROSSBOLT_MISFIRE)
        , timestamp_ns(0)
        , halt_latency_us(0)
        , keys_wiped(false)
        , sector_stopped(false)
    {}
};

class REFLEX {
public:
    REFLEX() : halted_sectors_(0), halt_broadcast_(false) {}
    
    // Called by REACT on BlackSeal isolation
    // Must complete in under 5ms
    HaltEvent fire(uint8_t sector_id,
                   uint8_t stack_id,
                   HaltReason reason) noexcept;

    bool is_halted(uint8_t sector_id) const noexcept;
    void reset(uint8_t sector_id) noexcept;  // manual override only
    
    // Emergency broadcast halt — all sectors
    void emergency_halt_all() noexcept;
    bool is_broadcast_halted() const noexcept;
    void clear_broadcast() noexcept;

private:
    std::atomic<uint8_t> halted_sectors_{0};      // bitmask — 7 sectors
    std::atomic<bool>    halt_broadcast_{false};  // emergency all-halt
    
    static constexpr uint8_t MAX_SECTORS = 7;
    
    void wipe_sector_keys(uint8_t sector_id) noexcept;
};

} // namespace REACT
