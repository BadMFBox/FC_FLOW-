#pragma once

#include "fc_flow/crypto/crossbolt_lock.hpp"
#include <expected>
#include <string_view>

namespace fc_flow::crypto {

enum class CrossBoltError : uint8_t {
    PIN_A_MISALIGNED = 1,
    PIN_B_MISALIGNED = 2,
    BOLT_DID_NOT_FIRE = 3,
    CRYPTO_ENGINE_FAILURE = 4,
    STALE_LOCK = 5,
    REPLAY_DETECTED = 6,
};

std::string_view to_string(CrossBoltError err) noexcept;

std::expected<CrossBolt, CrossBoltError> fire_crossbolt(
    const BoltInput& input
) noexcept;

std::expected<void, CrossBoltError> verify_crossbolt(
    const CrossBolt& lock,
    const BoltInput& expected_input,
    uint64_t freshness_window_ms = 500
) noexcept;

} // namespace fc_flow::crypto
