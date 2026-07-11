#pragma once

#include "fc_flow/enforcement/monitus.hpp"
#include <string>
#include <string_view>

namespace fc_flow::monitus {

// ── OUTPUT FORMATTING — Three Modes ──────────────────────────────────
// TERMINAL: Live formatted output to stdout
//           Shows last N events, sector health, pass/fail counts, alerts
//
// LOG:      Append-only structured log line per event
//           timestamp | sector | stack | type | bolt_prefix
//
// ALERT:    Single-line alert string for escalation
//           Triggers when silence threshold exceeded or failure rate high
//           Routes to S5 AiZquad for action decision

// ── TERMINAL OUTPUT ──────────────────────────────────────────────────
// Format: [timestamp] S{sector}:{stack} {type} bolt:{prefix_hex}
// Example: [1234567890.123456] S2:PEU BOLT_FIRED bolt:a3f4e8b2c1d9f7e6
std::string format_terminal(const MonitusEvent& e) noexcept;

// ── LOG OUTPUT ───────────────────────────────────────────────────────
// Format: timestamp_ns|sector_id|stack_id|event_type|bolt_prefix_hex
// Example: 1234567890123456789|2|1|BOLT_FIRED|a3f4e8b2c1d9f7e6
std::string format_log(const MonitusEvent& e) noexcept;

// ── ALERT OUTPUT ─────────────────────────────────────────────────────
// Format: ALERT: {event_type} S{sector}:{stack} at {timestamp}
// Example: ALERT: SILENCE S2:PEU at 1234567890.123456
// Routes to S5 AiZquad for action decision
std::string format_alert(const MonitusEvent& e) noexcept;

// ── HELPER: EventType to string ──────────────────────────────────────
std::string_view event_type_to_string(EventType type) noexcept;

// ── HELPER: Stack ID to string ───────────────────────────────────────
// 0x01 → "PEU", 0x02 → "FLOWSTATION"
std::string_view stack_id_to_string(uint8_t stack_id) noexcept;

} // namespace fc_flow::monitus
