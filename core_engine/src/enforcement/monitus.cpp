#include "fc_flow/enforcement/monitus.hpp"
#include <sodium.h>
#include <cstring>
#include <cstdio>

namespace fc_flow::enforcement {

// ══════════════════════════════════════════════════════
// MonitusEvent
// ══════════════════════════════════════════════════════

MonitusEvent::MonitusEvent() noexcept {
    sodium_memzero(this, sizeof(MonitusEvent));
}

void MonitusEvent::wipe() noexcept {
    sodium_memzero(this, sizeof(MonitusEvent));
}

// ══════════════════════════════════════════════════════
// Bolt Event Witnessing
// ══════════════════════════════════════════════════════

void Monitus::witness_bolt_fired(
    uint8_t sector_id,
    uint64_t flow_cycle,
    const crypto::CrossBolt& lock
) noexcept {
    size_t idx = _ring_index.fetch_add(1, std::memory_order_relaxed) % 50;
    
    _event_ring[idx].type = MonitusEventType::BOLT_FIRED;
    _event_ring[idx].sector_id = sector_id;
    _event_ring[idx].timestamp_ms = lock.timestamp_ns / 1000000ULL;
    _event_ring[idx].flow_cycle = flow_cycle;
    
    std::memcpy(_event_ring[idx].bolt_signature.data(), lock.bolt.data(), 32);
    
    _event_counts[0].fetch_add(1, std::memory_order_relaxed);
}

void Monitus::witness_bolt_failed(
    uint8_t sector_id,
    uint64_t flow_cycle,
    MonitusEventType failure_type
) noexcept {
    size_t idx = _ring_index.fetch_add(1, std::memory_order_relaxed) % 50;
    
    _event_ring[idx].type = failure_type;
    _event_ring[idx].sector_id = sector_id;
    _event_ring[idx].flow_cycle = flow_cycle;
    
    _event_counts[static_cast<uint8_t>(failure_type)].fetch_add(1, std::memory_order_relaxed);
}

void Monitus::witness_silence(
    uint8_t sector_id,
    uint64_t flow_cycle
) noexcept {
    size_t idx = _ring_index.fetch_add(1, std::memory_order_relaxed) % 50;
    
    _event_ring[idx].type = MonitusEventType::SILENCE;
    _event_ring[idx].sector_id = sector_id;
    _event_ring[idx].flow_cycle = flow_cycle;
    
    _event_counts[5].fetch_add(1, std::memory_order_relaxed);
}

std::array<MonitusEvent, 50> Monitus::get_recent_events() const noexcept {
    return _event_ring;
}

uint64_t Monitus::get_event_count(MonitusEventType type) const noexcept {
    return _event_counts[static_cast<uint8_t>(type)].load(std::memory_order_relaxed);
}

// ══════════════════════════════════════════════════════
// Miss Rate Alerts
// ══════════════════════════════════════════════════════

bool Monitus::push_miss_alert(uint8_t sector_id, uint8_t level, float miss_rate) noexcept {
    size_t head = _alert_head.load(std::memory_order_relaxed);
    size_t next = (head + 1) & ALERT_RING_MASK;
    
    // Full — drop silently rather than stall the hot path
    if (next == _alert_tail.load(std::memory_order_acquire)) {
        _alert_dropped.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    _alert_buf[head] = AlertEntry{sector_id, level, miss_rate, true};
    _alert_head.store(next, std::memory_order_release);
    return true;
}

void Monitus::drain_alerts() noexcept {
    size_t tail = _alert_tail.load(std::memory_order_relaxed);
    while (tail != _alert_head.load(std::memory_order_acquire)) {
        AlertEntry& e = _alert_buf[tail];
        if (e.level == 2) {
            printf("[Monitus] CRITICAL — S%d miss rate: %.1f%% — BLACKSEAL TRIGGERED\n",
                   e.sector_id, e.miss_rate * 100.f);
        } else {
            printf("[Monitus] WARNING — S%d miss rate: %.1f%%\n",
                   e.sector_id, e.miss_rate * 100.f);
        }
        e.valid = false;
        tail = (tail + 1) & ALERT_RING_MASK;
        _alert_tail.store(tail, std::memory_order_release);
    }
    
    uint32_t d = _alert_dropped.exchange(0, std::memory_order_relaxed);
    if (d > 0) {
        printf("[Monitus] %u alerts dropped (ring full)\n", d);
    }
}

size_t Monitus::pending_alerts() const noexcept {
    size_t h = _alert_head.load(std::memory_order_acquire);
    size_t t = _alert_tail.load(std::memory_order_acquire);
    return (h - t) & ALERT_RING_MASK;
}

} // namespace fc_flow::enforcement
