// FuelLine — STUB
#include "fuel_line.hpp"
#include <cstdio>

namespace fc_flow::fuel {

bool FuelLine::start(uint16_t port) noexcept {
    live_.store(true, std::memory_order_release);
    printf("[FuelLine] Live on :%d — STUB\n", port);
    return true;
}
void FuelLine::stop() noexcept {
    live_.store(false, std::memory_order_release);
    printf("[FuelLine] Stopped\n");
}
bool FuelLine::send(const FuelPacket& p) noexcept {
    if (!is_live()) return false;
    printf("[FuelLine] S%d → S%d loop=%d — STUB\n",
           p.src_sector, p.dst_sector, p.loop);
    return true;
}
bool FuelLine::is_live() const noexcept {
    return live_.load(std::memory_order_acquire);
}

} // namespace fc_flow::fuel
