// FuelLine — Inter-sector relay — STUB
#include "fc_flow/fuel/fuel_line.hpp"
#include <cstdio>

namespace fc_flow::fuel {

bool FuelLine::start(uint16_t port) noexcept {
    live_.store(true, std::memory_order_release);
    printf("[FuelLine] Started on :%d — STUB\n", port);
    return true;
}

void FuelLine::stop() noexcept {
    live_.store(false, std::memory_order_release);
    printf("[FuelLine] Stopped\n");
}

bool FuelLine::send(const FuelPacket& packet) noexcept {
    if (!is_live()) return false;
    printf("[FuelLine] S%d → S%d — STUB\n",
           packet.src_sector, packet.dst_sector);
    return true;
}

bool FuelLine::is_live() const noexcept {
    return live_.load(std::memory_order_acquire);
}

} // namespace fc_flow::fuel
