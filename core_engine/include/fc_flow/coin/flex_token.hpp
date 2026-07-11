#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <string_view>

namespace fc_flow::coin {

// ── FLEXTOKEN — PIN B ONLY ───────────────────────────────────────────
// SHA-256 unkeyed challenge/response proof
// Flows opposite direction from Undisputed Coin
// Cross-validates at every loop (default: every 3 loops, configurable)
//
// Heterogeneous primitives:
//   - Undisputed: HMAC-SHA256 (keyed)   — requires master_secret
//   - FlexToken:  SHA-256 (unkeyed)     — no key dependency
//
// Compromising master_secret cannot forge both witnesses simultaneously

struct alignas(8) FlexToken {
    std::array<uint8_t, 32> pin_b;       // SHA-256 unkeyed proof
    uint64_t timestamp_ns;               // nanosecond tick salt — self-destructs
    uint8_t sector_id;                   // 0-6 (S0-S6)
    uint8_t stack_id;                    // 0x01 PEU / 0x02 FLOWSTATION
    uint8_t reserved[6];

    FlexToken() noexcept;
    void wipe() noexcept;
};

static_assert(sizeof(FlexToken) == 48, "FlexToken must be 48 bytes");
static_assert(alignof(FlexToken) == 8, "FlexToken must be 8-byte aligned");

// ── FLEX INPUT ───────────────────────────────────────────────────────
struct FlexInput {
    uint64_t flow_cycle;                     // Must match Undisputed's cycle
    uint8_t sector_id;
    uint8_t stack_id;
    std::array<uint8_t, 32> challenge_hash;  // Fresh per loop, never reused

    void wipe() noexcept;
};

// ── ERRORS (reuse CoinError from undisputed_coin.hpp) ───────────────
enum class FlexError : uint8_t {
    CRYPTO_ENGINE_FAILURE = 1,
    PIN_B_MISALIGNED = 2,
    STALE_TOKEN = 3,
    INVALID_SECTOR = 4,
    INVALID_STACK = 5,
    CYCLE_DESYNC = 6,
};

std::string_view to_string(FlexError err) noexcept;

// ── MINT — Generate Pin B ────────────────────────────────────────────
std::expected<FlexToken, FlexError> mint_flex(
    const FlexInput& input
) noexcept;

// ── VERIFY — Validate Pin B ──────────────────────────────────────────
std::expected<void, FlexError> verify_flex(
    const FlexToken& token,
    const FlexInput& expected_input,
    uint64_t freshness_window_ms = 500
) noexcept;

} // namespace fc_flow::coin
