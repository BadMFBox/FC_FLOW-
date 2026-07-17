#pragma once
// FuelLine — Inter-sector relay (port 5557)
// 8CV figure-8 routing
// STUB — awaiting full implementation

#include <cstdint>
#include <atomic>

namespace fc_flow::fuel {

constexpr uint16_t FUEL_LINE_PORT = 5557;

struct FuelPacket {
    uint8_t  src_sector;
    uint8_t  dst_sector;
    uint8_t  stack_id;
    uint64_t timestamp_ns;
    uint8_t  payload[32];  // fixed size — no heap
};

class FuelLine {
public:
    bool start(uint16_t port = FUEL_LINE_PORT) noexcept;
    void stop() noexcept;
    bool send(const FuelPacket& packet) noexcept;
    bool is_live() const noexcept;

private:
    std::atomic<bool> live_{false};
    int server_fd_{-1};
};

} // namespace fc_flow::fuel
