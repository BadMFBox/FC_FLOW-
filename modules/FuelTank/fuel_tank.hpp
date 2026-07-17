#pragma once
// FuelTank — Shared fuel reserve
// Warm/cold/reactive/stable memory tiers
// Sectors draw on demand — tank refills continuously
// STUB

#include <cstdint>
#include <atomic>
#include <array>

namespace fc_flow::fuel {

constexpr uint8_t MAX_SECTORS = 7;

enum class TankTier : uint8_t { WARM=0, COLD=1, REACTIVE=2, STABLE=3 };

struct FuelReserve {
    uint8_t  sector_id  = 0;
    TankTier tier       = TankTier::COLD;
    uint64_t capacity   = 1000;
};

class FuelTank {
public:
    bool     fill(uint8_t sector_id, uint64_t amount) noexcept;
    uint64_t draw(uint8_t sector_id, uint64_t amount) noexcept;
    TankTier tier(uint8_t sector_id)            const noexcept;
    void     promote(uint8_t sector_id)               noexcept;
    void     demote(uint8_t sector_id)                noexcept;
    uint64_t level(uint8_t sector_id)           const noexcept;
private:
    std::array<FuelReserve, MAX_SECTORS>         reserves_{};
    std::array<std::atomic<uint64_t>, MAX_SECTORS> levels_{};
};

} // namespace fc_flow::fuel
