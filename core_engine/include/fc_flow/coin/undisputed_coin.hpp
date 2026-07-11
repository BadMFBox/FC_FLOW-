#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <string_view>

namespace fc_flow::coin {

// ── UNDISPUTED COIN — PIN A ONLY ─────────────────────────────────────
// HMAC-SHA256 keyed proof bound to:
//   - master_secret (32 bytes)
//   - hardware_dna (64 bytes)
//   - flow_cycle (uint64_t)
//   - sector_id (uint8_t 0-6)
//   - stack_id (uint8_t 0x01 PEU / 0x02 FLOWSTATION)
//   - entropy (32 bytes)
//   - tick_salt (timestamp + OS entropy — self-destructs)
//
// Output: 32-byte HMAC-SHA256 proof (Pin A)
// Timestamp salt expires — proof cannot be replicated after window closes

struct alignas(8) UndisputedCoin {
    std::array<uint8_t, 32> pin_a;       // HMAC-SHA256 keyed proof
    uint64_t timestamp_ns;               // nanosecond tick salt — self-destructs
    uint8_t sector_id;                   // 0-6 (S0-S6)
    uint8_t stack_id;                    // 0x01 PEU / 0x02 FLOWSTATION
    uint8_t reserved[6];

    UndisputedCoin() noexcept;
    void wipe() noexcept;
};

static_assert(sizeof(UndisputedCoin) == 48, "UndisputedCoin must be 48 bytes");
static_assert(alignof(UndisputedCoin) == 8, "UndisputedCoin must be 8-byte aligned");

// ── COIN INPUT ───────────────────────────────────────────────────────
struct CoinInput {
    std::array<uint8_t, 32> master_secret;
    std::array<uint8_t, 64> hardware_dna;
    uint64_t flow_cycle;
    uint8_t sector_id;
    uint8_t stack_id;
    std::array<uint8_t, 32> entropy;

    void wipe() noexcept;
};

// ── ERRORS ───────────────────────────────────────────────────────────
enum class CoinError : uint8_t {
    CRYPTO_ENGINE_FAILURE = 1,
    PIN_A_MISALIGNED = 2,
    STALE_COIN = 3,
    INVALID_SECTOR = 4,
    INVALID_STACK = 5,
};

std::string_view to_string(CoinError err) noexcept;

// ── MINT — Generate Pin A ────────────────────────────────────────────
std::expected<UndisputedCoin, CoinError> mint_undisputed(
    const CoinInput& input
) noexcept;

// ── VERIFY — Validate Pin A ──────────────────────────────────────────
std::expected<void, CoinError> verify_undisputed(
    const UndisputedCoin& coin,
    const CoinInput& expected_input,
    uint64_t freshness_window_ms = 500
) noexcept;

} // namespace fc_flow::coin
