#pragma once

#include <array>
#include <cstdint>

namespace fc_flow::crypto {

struct alignas(8) CrossBolt {
    std::array<uint8_t, 32> pin_a;
    std::array<uint8_t, 32> pin_b;
    std::array<uint8_t, 32> bolt;
    uint64_t timestamp_ns;
    uint8_t sector_id;
    uint8_t stack_id;
    uint8_t reserved[6];

    CrossBolt() noexcept;
    void wipe() noexcept;
};

static_assert(sizeof(CrossBolt) == 112, "CrossBolt must be 112 bytes");
static_assert(alignof(CrossBolt) == 8, "CrossBolt must be 8-byte aligned");

struct BoltInput {
    std::array<uint8_t, 32> master_secret;
    std::array<uint8_t, 64> hardware_dna;
    uint64_t flow_cycle;
    uint8_t sector_id;
    uint8_t stack_id;
    std::array<uint8_t, 32> entropy;
    uint64_t tick_salt;

    void wipe() noexcept;
};

} // namespace fc_flow::crypto
