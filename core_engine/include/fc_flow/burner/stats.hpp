#pragma once
#include <atomic>
#include <unordered_map>
#include <cstdint>

namespace fc_flow::burner {

enum class RejectReason : uint8_t {
    NOT_FOUND = 0,
    EXPIRED = 1,
    CONSUMED = 2,
    WRONG_SECTOR = 3,
    BAD_HMAC = 4,
    BURNED = 5,
    REPLAYED = 6,
    RATE_LIMIT = 7,
    NO_FUEL = 8
};

struct GateStats {
    std::atomic<uint64_t> tokens_born{0};
    std::atomic<uint64_t> tokens_admitted{0};
    std::atomic<uint64_t> tokens_rejected{0};
    std::atomic<uint64_t> tokens_expired{0};
    std::atomic<uint64_t> tokens_wiped{0};
    std::atomic<uint64_t> burns_triggered{0};
    std::atomic<uint64_t> rate_limits_hit{0};
    
    std::unordered_map<uint32_t, uint64_t> tokens_per_sector;
    std::unordered_map<RejectReason, uint64_t> rejections_by_reason;
    
    void record_birth(uint32_t sector_id) noexcept;
    void record_admit() noexcept;
    void record_reject(RejectReason reason) noexcept;
    void record_expire() noexcept;
    void record_wipe(uint64_t count) noexcept;
    void record_burn() noexcept;
    void record_rate_limit() noexcept;
};

} // namespace fc_flow::burner
