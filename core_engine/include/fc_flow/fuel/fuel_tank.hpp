#pragma once
// FuelTank — Shared fuel reserve per sector
// Warm/cold memory tier — sectors draw on demand
// STUB — awaiting full implementation

#include <cstdint>
#include <atomic>
#include <array>

namespace fc_flow::fuel {

constexpr uint8_t MAX_SECTORS = 7;

enum class TankTier : uint8_t {
    WARM     = 0,  // active — in RAM — mlock'd
    COLD     = 1,  // inactive — on storage
    REACTIVE = 2,  // promoted from cold on threat
    STABLE   = 3   // demoted from warm after clean cycles
};

struct FuelReserve {
    uint8_t  sector_id;
    TankTier tier;
    uint64_t level;       // current fuel level
    uint64_t capacity;    // max capacity
    uint64_t last_draw_ns;
};

class FuelTank {
public:
    bool     fill(uint8_t sector_id, uint64_t amount) noexcept;
    uint64_t draw(uint8_t sector_id, uint64_t amount) noexcept;
    TankTier tier(uint8_t sector_id) const noexcept;
    void     promote(uint8_t sector_id) noexcept;  // cold → warm
    void     demote(uint8_t sector_id)  noexcept;  // warm → cold

private:
    std::array<FuelReserve, MAX_SECTORS> reserves_{};
    std::array<std::atomic<uint64_t>, MAX_SECTORS> levels_{};
};

} // namespace fc_flow::fuel
