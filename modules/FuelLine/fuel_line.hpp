#pragma once
// FuelLine — Inter-sector relay
// Port 5557 — 8CV figure-8 routing
// STUB

#include <cstdint>
#include <atomic>
#include <array>

namespace fc_flow::fuel {

constexpr uint16_t FUEL_LINE_PORT = 5557;

struct FuelPacket {
    uint8_t  src_sector;
    uint8_t  dst_sector;
    uint8_t  stack_id;      // 0x01 PEU / 0x02 FLOWSTATION
    uint8_t  loop;          // 0=top(S0-S3) 1=bottom(S0-S4-S5-S6)
    uint64_t timestamp_ns;
    uint8_t  payload[32];   // fixed — no heap
};

class FuelLine {
public:
    bool start(uint16_t port = FUEL_LINE_PORT) noexcept;
    void stop()                                noexcept;
    bool send(const FuelPacket& pkt)           noexcept;
    bool is_live()                       const noexcept;
private:
    std::atomic<bool> live_{false};
    int               fd_{-1};
};

} // namespace fc_flow::fuel
