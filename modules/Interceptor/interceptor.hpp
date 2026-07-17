#pragma once
// FC_FLOW S4 — Interceptor
// Role: Radar and Target Registry
// Port: 6666
// STUB — awaiting full implementation

#include <cstdint>
#include <atomic>

namespace fc_flow::sectors {

class Interceptor {
public:
    static constexpr uint8_t  SECTOR_ID = 4;
    static constexpr uint16_t PORT      = 6666;

    bool    boot()         noexcept;
    void    shutdown()     noexcept;
    bool    is_live() const noexcept;
    uint8_t sector_id() const noexcept { return SECTOR_ID; }

private:
    std::atomic<bool> live_{false};
};

} // namespace fc_flow::sectors
