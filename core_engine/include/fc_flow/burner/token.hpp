#pragma once
#include <array>
#include <cstdint>
#include <chrono>

namespace fc_flow::burner {

constexpr size_t TOKEN_SIZE = 32;
constexpr size_t NONCE_SIZE = 16;

enum class TokenStatus : uint8_t {
    LIVE = 0,
    CONSUMED = 1,
    EXPIRED = 2,
    BURNED = 3
};

struct Token {
    std::array<uint8_t, TOKEN_SIZE> value;
    std::array<uint8_t, NONCE_SIZE> nonce;
    uint32_t sector_id;
    uint64_t born_at_ns;
    uint64_t sequence;
    TokenStatus status;
    std::chrono::steady_clock::time_point created_at;
    
    bool is_live(uint64_t max_age_ns) const noexcept;
    uint64_t age_ns() const noexcept;
    uint64_t ttl_ns(uint64_t max_age_ns) const noexcept;
    
    static void derive_hmac(
        const uint8_t* gate_key,
        uint32_t sector_id,
        uint64_t born_at_ns,
        const uint8_t* nonce,
        uint8_t* out_token
    ) noexcept;
    
    static bool verify_hmac(
        const uint8_t* gate_key,
        const uint8_t* token_value,
        uint32_t sector_id,
        uint64_t born_at_ns,
        const uint8_t* nonce
    ) noexcept;
};

struct TokenKey {
    std::array<uint8_t, TOKEN_SIZE> bytes;
    
    bool operator<(const TokenKey& other) const noexcept {
        return bytes < other.bytes;
    }
};

} // namespace fc_flow::burner
