#pragma once
#include <atomic>
#include <mutex>
#include <deque>
#include <chrono>
#include <cstdint>

namespace fc_flow::burner {

class FuelFlow {
public:
    std::atomic<uint32_t> strike_count{0};
    std::atomic<bool> burned{false};
    
    bool check_rate_limit(uint32_t max_requests, uint64_t window_ns) noexcept;
    void record_request() noexcept;
    uint32_t increment_strikes() noexcept;
    void reset_strikes() noexcept;
    void burn() noexcept;
    
private:
    std::mutex rate_mutex;
    std::deque<std::chrono::steady_clock::time_point> request_times;
};

} // namespace fc_flow::burner
