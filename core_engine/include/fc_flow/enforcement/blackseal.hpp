#pragma once

#include <atomic>
#include <cstdint>

namespace REACT {

// ═══════════════════════════════════════════════════════
// BlackSeal — Sector isolation on MISFIRE
// One job: receive misfire signal, isolate sector, log it
// Sub-2ms guaranteed
// ═══════════════════════════════════════════════════════

enum class SealReason : uint8_t {
    MISFIRE_PIN_A = 0,
    MISFIRE_PIN_B = 1,
    MISFIRE_BOLT  = 2,
    SILENCE       = 3,
    REPLAY        = 4
};

struct SealEvent {
    uint8_t    sector_id;
    uint8_t    stack_id;
    SealReason reason;
    uint64_t   timestamp_ns;
    bool       isolated;
    
    SealEvent()
        : sector_id(0)
        , stack_id(0)
        , reason(SealReason::MISFIRE_PIN_A)
        , timestamp_ns(0)
        , isolated(false)
    {}
};

class BlackSeal {
public:
    BlackSeal() : isolated_sectors_(0) {}
    
    // Called by Monitus on MISFIRE
    // Returns confirmation in under 2ms
    SealEvent isolate(uint8_t sector_id, 
                      uint8_t stack_id,
                      SealReason reason) noexcept;
    
    bool is_isolated(uint8_t sector_id) const noexcept;
    void release(uint8_t sector_id) noexcept;  // manual override only

private:
    std::atomic<uint8_t> isolated_sectors_{0};  // bitmask — 7 sectors (0-6)
    
    static constexpr uint8_t MAX_SECTORS = 7;
};

} // namespace REACT
