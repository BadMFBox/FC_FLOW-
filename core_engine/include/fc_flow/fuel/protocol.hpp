#pragma once
// Protocol — 8CV figure-8 routing rules
// STUB — awaiting full implementation

#include <cstdint>

namespace fc_flow::fuel::protocol {

// Figure-8 routing — 8CV
// S0 → S1 → S2 → S3 → S0 (top loop)
// S0 → S4 → S5 → S6 → S0 (bottom loop)
// Crossover at S0 (SovereignFuel) and S3 (Vortex)

constexpr uint8_t ROUTE_TOP[]    = {0, 1, 2, 3, 0};
constexpr uint8_t ROUTE_BOTTOM[] = {0, 4, 5, 6, 0};

uint8_t next_sector(uint8_t current, bool top_loop) noexcept;
bool    is_crossover(uint8_t sector_id) noexcept;

} // namespace fc_flow::fuel::protocol
