#pragma once
#include "fc_flow/burner/token.hpp"
#include "fc_flow/burner/stats.hpp"
#include "fc_flow/burner/fuel_flow.hpp"
#include "fc_flow/burner/ram_wiper.hpp"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <optional>
#include <array>

namespace fc_flow::burner {

struct GateResult {
    bool success;
    std::optional<std::array<uint8_t, TOKEN_SIZE>> token_value;
    std::optional<RejectReason> reject_reason;
    uint32_t strike_count;
    bool burn_triggered;
};

class PredatorGate {
public:
    explicit PredatorGate(bool lock_memory = true);
    ~PredatorGate() noexcept;
    
    GateResult birth_token(uint32_t sector_id) noexcept;
    GateResult admit(uint32_t sector_id, const uint8_t* token_value) noexcept;
    
    void trigger_burn() noexcept;
    void remove_session(uint32_t sector_id) noexcept;
    bool is_burning() const noexcept;
    const GateStats& get_stats() const noexcept;
    void destroy() noexcept;
    
private:
    static constexpr uint64_t TOKEN_MAX_AGE_NS = 5'000'000'000ULL; // 5 seconds
    static constexpr uint32_t MAX_TOKENS_PER_SECTOR = 1000;
    static constexpr uint32_t MAX_STRIKES = 3;
    static constexpr uint32_t MAX_RATE_REQUESTS = 100;
    static constexpr uint64_t RATE_WINDOW_NS = 1'000'000'000ULL; // 1 second
    
    std::array<uint8_t, 32> gate_key_;
    std::map<TokenKey, Token> tokens_;
    std::unordered_map<uint32_t, uint32_t> live_count_;
    std::unordered_map<uint32_t, std::unique_ptr<FuelFlow>> fuel_flows_;
    std::unordered_map<uint32_t, std::unordered_set<uint64_t>> seen_sequences_;
    
    GateStats stats_;
    RAMWiper wiper_;
    std::mutex mutex_;
    std::atomic<bool> burning_{false};
    uint64_t sequence_{0};
    
    GateResult reject(RejectReason reason, FuelFlow* fuel) noexcept;
    void expire_old_tokens() noexcept;
    void burn_all_tokens() noexcept;
    void decrement_live_count(uint32_t sector_id) noexcept;
    FuelFlow* get_or_create_fuel(uint32_t sector_id) noexcept;
};

} // namespace fc_flow::burner
