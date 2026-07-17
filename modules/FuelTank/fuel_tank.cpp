// FuelTank — STUB
#include "fuel_tank.hpp"
#include <cstdio>

namespace fc_flow::fuel {

bool FuelTank::fill(uint8_t s, uint64_t a) noexcept {
    if (s >= MAX_SECTORS) return false;
    levels_[s].fetch_add(a, std::memory_order_relaxed);
    return true;
}
uint64_t FuelTank::draw(uint8_t s, uint64_t a) noexcept {
    if (s >= MAX_SECTORS) return 0;
    return levels_[s].fetch_sub(a, std::memory_order_relaxed);
}
TankTier FuelTank::tier(uint8_t s) const noexcept {
    if (s >= MAX_SECTORS) return TankTier::COLD;
    return reserves_[s].tier;
}
void FuelTank::promote(uint8_t s) noexcept {
    if (s >= MAX_SECTORS) return;
    reserves_[s].tier = TankTier::WARM;
    printf("[FuelTank] S%d → WARM\n", s);
}
void FuelTank::demote(uint8_t s) noexcept {
    if (s >= MAX_SECTORS) return;
    reserves_[s].tier = TankTier::COLD;
    printf("[FuelTank] S%d → COLD\n", s);
}
uint64_t FuelTank::level(uint8_t s) const noexcept {
    if (s >= MAX_SECTORS) return 0;
    return levels_[s].load(std::memory_order_relaxed);
}

} // namespace fc_flow::fuel
