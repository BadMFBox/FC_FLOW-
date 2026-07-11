#include "fc_flow/crypto/crossbolt.hpp"
#include <sodium.h>
#include <cstring>
#include <time.h>

namespace fc_flow::crypto {

static bool init_libsodium() noexcept {
    static bool initialized = false;
    if (!initialized) {
        if (sodium_init() < 0) return false;
        initialized = true;
    }
    return true;
}

CrossBolt::CrossBolt() noexcept {
    sodium_memzero(this, sizeof(CrossBolt));
}

void CrossBolt::wipe() noexcept {
    sodium_memzero(this, sizeof(CrossBolt));
}

void BoltInput::wipe() noexcept {
    sodium_memzero(this, sizeof(BoltInput));
}

std::string_view to_string(CrossBoltError err) noexcept {
    switch (err) {
        case CrossBoltError::PIN_A_MISALIGNED:      return "PIN_A_MISALIGNED";
        case CrossBoltError::PIN_B_MISALIGNED:      return "PIN_B_MISALIGNED";
        case CrossBoltError::BOLT_DID_NOT_FIRE:     return "BOLT_DID_NOT_FIRE";
        case CrossBoltError::CRYPTO_ENGINE_FAILURE: return "CRYPTO_ENGINE_FAILURE";
        case CrossBoltError::STALE_LOCK:            return "STALE_LOCK";
        case CrossBoltError::REPLAY_DETECTED:       return "REPLAY_DETECTED";
    }
    return "UNKNOWN";
}

static uint64_t current_time_ns() noexcept {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

std::expected<CrossBolt, CrossBoltError> fire_crossbolt(
    const BoltInput& input
) noexcept {
    if (!init_libsodium()) {
        return std::unexpected(CrossBoltError::CRYPTO_ENGINE_FAILURE);
    }

    CrossBolt lock;
    lock.timestamp_ns = current_time_ns();
    if (lock.timestamp_ns == 0) {
        return std::unexpected(CrossBoltError::CRYPTO_ENGINE_FAILURE);
    }
    lock.sector_id = input.sector_id;
    lock.stack_id = input.stack_id;

    // PIN A: HMAC-SHA256 (keyed witness)
    constexpr size_t pin_a_input_size = 32 + 64 + 8 + 1 + 1 + 32 + 8;
    std::array<uint8_t, pin_a_input_size> pin_a_input;
    size_t offset = 0;

    std::memcpy(pin_a_input.data() + offset, input.master_secret.data(), 32);
    offset += 32;
    std::memcpy(pin_a_input.data() + offset, input.hardware_dna.data(), 64);
    offset += 64;
    std::memcpy(pin_a_input.data() + offset, &input.flow_cycle, 8);
    offset += 8;
    pin_a_input[offset++] = input.sector_id;
    pin_a_input[offset++] = input.stack_id;
    std::memcpy(pin_a_input.data() + offset, input.entropy.data(), 32);
    offset += 32;
    std::memcpy(pin_a_input.data() + offset, &input.tick_salt, 8);

    if (crypto_auth_hmacsha256(lock.pin_a.data(), pin_a_input.data(), 
                               pin_a_input.size(), input.master_secret.data()) != 0) {
        lock.wipe();
        return std::unexpected(CrossBoltError::PIN_A_MISALIGNED);
    }

    sodium_memzero(pin_a_input.data(), pin_a_input.size());

    // PIN B: SHA-256 with timestamp binding
    constexpr size_t pin_b_input_size = 8 + 1 + 1 + 32 + 8;
    std::array<uint8_t, pin_b_input_size> pin_b_input;
    offset = 0;

    std::memcpy(pin_b_input.data() + offset, &input.flow_cycle, 8);
    offset += 8;
    pin_b_input[offset++] = input.sector_id;
    pin_b_input[offset++] = input.stack_id;
    std::memcpy(pin_b_input.data() + offset, input.entropy.data(), 32);
    offset += 32;
    std::memcpy(pin_b_input.data() + offset, &lock.timestamp_ns, 8);

    if (crypto_hash_sha256(lock.pin_b.data(), pin_b_input.data(), 
                          pin_b_input.size()) != 0) {
        lock.wipe();
        return std::unexpected(CrossBoltError::PIN_B_MISALIGNED);
    }

    sodium_memzero(pin_b_input.data(), pin_b_input.size());

    // BOLT: Binding HMAC
    constexpr size_t bolt_input_size = 32 + 32 + 8 + 1 + 1;
    std::array<uint8_t, bolt_input_size> bolt_input;
    offset = 0;

    std::memcpy(bolt_input.data() + offset, lock.pin_a.data(), 32);
    offset += 32;
    std::memcpy(bolt_input.data() + offset, lock.pin_b.data(), 32);
    offset += 32;
    std::memcpy(bolt_input.data() + offset, &lock.timestamp_ns, 8);
    offset += 8;
    bolt_input[offset++] = lock.sector_id;
    bolt_input[offset++] = lock.stack_id;

    if (crypto_auth_hmacsha256(lock.bolt.data(), bolt_input.data(), 
                               bolt_input.size(), input.master_secret.data()) != 0) {
        lock.wipe();
        return std::unexpected(CrossBoltError::BOLT_DID_NOT_FIRE);
    }

    sodium_memzero(bolt_input.data(), bolt_input.size());
    return lock;
}

std::expected<void, CrossBoltError> verify_crossbolt(
    const CrossBolt& lock,
    const BoltInput& expected_input,
    uint64_t freshness_window_ms
) noexcept {
    if (!init_libsodium()) {
        return std::unexpected(CrossBoltError::CRYPTO_ENGINE_FAILURE);
    }

    uint64_t now_ns = current_time_ns();
    if (now_ns == 0) {
        return std::unexpected(CrossBoltError::CRYPTO_ENGINE_FAILURE);
    }

    uint64_t age_ns = (now_ns > lock.timestamp_ns) ? (now_ns - lock.timestamp_ns) : 0;
    uint64_t freshness_window_ns = freshness_window_ms * 1000000ULL;
    
    if (age_ns > freshness_window_ns) {
        return std::unexpected(CrossBoltError::STALE_LOCK);
    }

    if (lock.sector_id != expected_input.sector_id) {
        return std::unexpected(CrossBoltError::BOLT_DID_NOT_FIRE);
    }
    if (lock.stack_id != expected_input.stack_id) {
        return std::unexpected(CrossBoltError::BOLT_DID_NOT_FIRE);
    }

    // Verify PIN A
    constexpr size_t pin_a_input_size = 32 + 64 + 8 + 1 + 1 + 32 + 8;
    std::array<uint8_t, pin_a_input_size> pin_a_input;
    size_t offset = 0;

    std::memcpy(pin_a_input.data() + offset, expected_input.master_secret.data(), 32);
    offset += 32;
    std::memcpy(pin_a_input.data() + offset, expected_input.hardware_dna.data(), 64);
    offset += 64;
    std::memcpy(pin_a_input.data() + offset, &expected_input.flow_cycle, 8);
    offset += 8;
    pin_a_input[offset++] = expected_input.sector_id;
    pin_a_input[offset++] = expected_input.stack_id;
    std::memcpy(pin_a_input.data() + offset, expected_input.entropy.data(), 32);
    offset += 32;
    std::memcpy(pin_a_input.data() + offset, &expected_input.tick_salt, 8);

    std::array<uint8_t, 32> expected_pin_a;
    if (crypto_auth_hmacsha256(expected_pin_a.data(), pin_a_input.data(), 
                               pin_a_input.size(), expected_input.master_secret.data()) != 0) {
        sodium_memzero(pin_a_input.data(), pin_a_input.size());
        return std::unexpected(CrossBoltError::CRYPTO_ENGINE_FAILURE);
    }

    sodium_memzero(pin_a_input.data(), pin_a_input.size());

    if (sodium_memcmp(expected_pin_a.data(), lock.pin_a.data(), 32) != 0) {
        return std::unexpected(CrossBoltError::PIN_A_MISALIGNED);
    }

    // Verify PIN B (with timestamp binding)
    constexpr size_t pin_b_input_size = 8 + 1 + 1 + 32 + 8;
    std::array<uint8_t, pin_b_input_size> pin_b_input;
    offset = 0;

    std::memcpy(pin_b_input.data() + offset, &expected_input.flow_cycle, 8);
    offset += 8;
    pin_b_input[offset++] = expected_input.sector_id;
    pin_b_input[offset++] = expected_input.stack_id;
    std::memcpy(pin_b_input.data() + offset, expected_input.entropy.data(), 32);
    offset += 32;
    std::memcpy(pin_b_input.data() + offset, &lock.timestamp_ns, 8);

    std::array<uint8_t, 32> expected_pin_b;
    if (crypto_hash_sha256(expected_pin_b.data(), pin_b_input.data(), 
                          pin_b_input.size()) != 0) {
        sodium_memzero(pin_b_input.data(), pin_b_input.size());
        return std::unexpected(CrossBoltError::CRYPTO_ENGINE_FAILURE);
    }

    sodium_memzero(pin_b_input.data(), pin_b_input.size());

    if (sodium_memcmp(expected_pin_b.data(), lock.pin_b.data(), 32) != 0) {
        return std::unexpected(CrossBoltError::PIN_B_MISALIGNED);
    }

    // Verify BOLT
    constexpr size_t bolt_input_size = 32 + 32 + 8 + 1 + 1;
    std::array<uint8_t, bolt_input_size> bolt_input;
    offset = 0;

    std::memcpy(bolt_input.data() + offset, lock.pin_a.data(), 32);
    offset += 32;
    std::memcpy(bolt_input.data() + offset, lock.pin_b.data(), 32);
    offset += 32;
    std::memcpy(bolt_input.data() + offset, &lock.timestamp_ns, 8);
    offset += 8;
    bolt_input[offset++] = lock.sector_id;
    bolt_input[offset++] = lock.stack_id;

    std::array<uint8_t, 32> expected_bolt;
    if (crypto_auth_hmacsha256(expected_bolt.data(), bolt_input.data(), 
                               bolt_input.size(), expected_input.master_secret.data()) != 0) {
        sodium_memzero(bolt_input.data(), bolt_input.size());
        return std::unexpected(CrossBoltError::CRYPTO_ENGINE_FAILURE);
    }

    sodium_memzero(bolt_input.data(), bolt_input.size());

    if (sodium_memcmp(expected_bolt.data(), lock.bolt.data(), 32) != 0) {
        return std::unexpected(CrossBoltError::BOLT_DID_NOT_FIRE);
    }

    return {};
}

} // namespace fc_flow::crypto
