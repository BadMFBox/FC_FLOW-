#pragma once
#include "fc_flow/crypto/crossbolt_lock.hpp"
#include <array>
#include <cstdint>
#include <cstddef>
#include <atomic>

namespace fc_flow::enforcement {

enum class MonitusEventType : uint8_t {
    BOLT_FIRED       = 0,
    PIN_A_MISALIGNED = 1,
    PIN_B_MISALIGNED = 2,
    BOLT_FAILED      = 3,
    STALE_LOCK       = 4,
    SILENCE          = 5,
};

struct alignas(8) MonitusEvent {
    MonitusEventType type;
    uint8_t sector_id;
    uint8_t padding1[6];
    uint64_t timestamp_ms;
    uint64_t flow_cycle;
    std::array<uint8_t, 32> bolt_signature;
    std::array<uint8_t, 32> reserved;
    
    MonitusEvent() noexcept;
    void wipe() noexcept;
};

static_assert(sizeof(MonitusEvent) == 88, "MonitusEvent must be 88 bytes");
static_assert(alignof(MonitusEvent) == 8, "MonitusEvent must be 8-byte aligned");

// ═══════════════════════════════════════════════════════
// Miss Rate Alert Entry
// ═══════════════════════════════════════════════════════

struct AlertEntry {
    uint8_t  sector_id = 0;
    uint8_t  level     = 0;   // 1=WARNING 2=CRITICAL
    float    miss_rate = 0.f;
    bool     valid     = false;
};

static constexpr size_t ALERT_RING_SIZE = 64;
static constexpr size_t ALERT_RING_MASK = ALERT_RING_SIZE - 1;

class Monitus {
public:
    // ── Bolt Event Witnessing ────────────────────────
    void witness_bolt_fired(
        uint8_t sector_id,
        uint64_t flow_cycle,
        const crypto::CrossBolt& lock
    ) noexcept;
    
    void witness_bolt_failed(
        uint8_t sector_id,
        uint64_t flow_cycle,
        MonitusEventType failure_type
    ) noexcept;
    
    void witness_silence(
        uint8_t sector_id,
        uint64_t flow_cycle
    ) noexcept;
    
    std::array<MonitusEvent, 50> get_recent_events() const noexcept;
    uint64_t get_event_count(MonitusEventType type) const noexcept;
    
    // ── Miss Rate Alerts ──────────────────────────────
    bool push_miss_alert(uint8_t sector_id, uint8_t level, float miss_rate) noexcept;
    void drain_alerts() noexcept;
    size_t pending_alerts() const noexcept;

private:
    // Bolt event witness storage
    std::array<MonitusEvent, 50> _event_ring{};
    std::atomic<size_t> _ring_index{0};
    std::array<std::atomic<uint64_t>, 6> _event_counts{};
    
    // Miss rate alert storage (lock-free SPSC)
    alignas(64) std::array<AlertEntry, ALERT_RING_SIZE> _alert_buf{};
    alignas(64) std::atomic<size_t>   _alert_head{0};
    alignas(64) std::atomic<size_t>   _alert_tail{0};
    alignas(64) std::atomic<uint32_t> _alert_dropped{0};
};

} // namespace fc_flow::enforcement
