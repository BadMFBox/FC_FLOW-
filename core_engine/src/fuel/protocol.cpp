// Protocol — 8CV figure-8 routing — STUB
#include "fc_flow/fuel/protocol.hpp"

namespace fc_flow::fuel::protocol {

uint8_t next_sector(uint8_t current, bool top_loop) noexcept {
    if (top_loop) {
        switch(current) {
            case 0: return 1;
            case 1: return 2;
            case 2: return 3;
            case 3: return 0;
            default: return 0;
        }
    } else {
        switch(current) {
            case 0: return 4;
            case 4: return 5;
            case 5: return 6;
            case 6: return 0;
            default: return 0;
        }
    }
}

bool is_crossover(uint8_t sector_id) noexcept {
    return sector_id == 0 || sector_id == 3;  // S0 and S3 Vortex
}

} // namespace fc_flow::fuel::protocol
