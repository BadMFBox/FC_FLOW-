// FuelTank — STUB
#include "fc_flow/fuel/fuel_tank.hpp"
#include <cstdio>

namespace fc_flow::fuel {

bool FuelTank::fill(uint8_t sector_id, uint64_t amount) noexcept {
    if (sector_id >= MAX_SECTORS) return false;
    levels_[sector_id].fetch_add(amount, std::memory_order_relaxed);
    return true;
}

uint64_t FuelTank::draw(uint8_t sector_id, uint64_t amount) noexcept {
    if (sector_id >= MAX_SECTORS) return 0;
    return levels_[sector_id].fetch_sub(amount, std::memory_order_relaxed);
}

TankTier FuelTank::tier(uint8_t sector_id) const noexcept {
    if (sector_id >= MAX_SECTORS) return TankTier::COLD;
    return reserves_[sector_id].tier;
}

void FuelTank::promote(uint8_t sector_id) noexcept {
    if (sector_id >= MAX_SECTORS) return;
    reserves_[sector_id].tier = TankTier::WARM;
    printf("[FuelTank] S%d promoted to WARM\n", sector_id);
}

void FuelTank::demote(uint8_t sector_id) noexcept {
    if (sector_id >= MAX_SECTORS) return;
    reserves_[sector_id].tier = TankTier::COLD;
    printf("[FuelTank] S%d demoted to COLD\n", sector_id);
}

} // namespace fc_flow::fuel
