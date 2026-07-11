#pragma once

#include <cstdint>
#include <array>
#include <chrono>
#include <string>

namespace fc_flow::burner {

// ═══════════════════════════════════════════════════════
// CROSS-VALIDATED TOKEN
// Undisputed (PIN_A) + Flex (PIN_B) both REAL
// Both calculated from seeds (ephemeral)
// Both must validate for true access
// Either fails → both dissolve, Master Key becomes bait
// ═══════════════════════════════════════════════════════

struct CrossValidatedToken {
    // ── UNDISPUTED (PIN_A) — Calculated from seed ──
    struct {
        std::array<uint8_t, 32> live_hash;  // Derived HMAC
        bool factor_one;                     // Must validate
        bool alive;                          // Seed not dissolved?
    } undisputed;
    
    // ── FLEX (PIN_B) — Calculated from flex_secret ──
    struct {
        std::array<uint8_t, 32> flex_hash;  // Derived HMAC
        bool factor_two;                     // Must validate
        bool alive;                          // Secret not dissolved?
    } flex;
    
    // ── Metadata ──
    uint8_t  source_room;
    uint8_t  target_room;
    uint64_t cycle_id;
    int64_t  issued_at;
    int64_t  expires_at;
    
    // ── Cross-validation ──
    bool is_valid() const {
        return undisputed.factor_one &&
               flex.factor_two &&
               undisputed.alive &&
               flex.alive;
    }
    
    bool is_expired() const {
        auto now = std::chrono::system_clock::now();
        int64_t now_ts = std::chrono::duration_cast<
            std::chrono::milliseconds>(
                now.time_since_epoch()).count();
        return now_ts > expires_at;
    }
};

// ── Room 5 Intake (CHECK-IN from honeypot) ──
struct Room5Intake {
    std::chrono::system_clock::time_point timestamp;
    std::array<uint8_t, 32> stolen_key;
    std::string status;      // "CHECK_IN"
    std::string next_step;   // "SAND_BOX"
    std::string attacker_id; // Fingerprint
};

} // namespace fc_flow::burner
