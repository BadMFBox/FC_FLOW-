#pragma once
// Eight_Core_Velocity (8CV) — FC_FLOW Engine
// 8 concurrent encode/decode loops at 3.7Hz
// 1.8 units per tick per core — 14.4 total
// STUB — awaiting full implementation

#include <atomic>
#include <cstdint>
#include <array>

namespace fc_flow::engine {

constexpr uint8_t  CORE_COUNT       = 8;
constexpr uint32_t TICK_US          = 270000;  // 3.7Hz
constexpr float    UNITS_PER_TICK   = 1.8f;

enum class CoreState : uint8_t { IDLE=0, RUNNING=1, HALTED=2, ERROR=3 };

struct CoreStatus {
    uint8_t   core_id;
    CoreState state;
    uint64_t  tick_count;
    uint64_t  last_tick_ns;
    float     velocity;
};

class EightCV {
public:
    bool        start()                          noexcept;
    void        stop()                           noexcept;
    bool        is_running()               const noexcept;
    CoreStatus  core_status(uint8_t id)    const noexcept;
    uint64_t    total_ticks()              const noexcept;
private:
    std::atomic<bool>     running_{false};
    std::atomic<uint64_t> total_ticks_{0};
    std::array<std::atomic<uint8_t>, CORE_COUNT> core_states_{};
};

} // namespace fc_flow::engine
