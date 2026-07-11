#ifndef FC_FLOW_CENSOR_HPP
#define FC_FLOW_CENSOR_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>

// ═══════════════════════════════════════════════════════
// SOVEREIGN PEU — CENSOR
// Better than SIGKILL
// Warrant + Green Light required — Judge authorized
// Zero tolerance — self defense exception
// Case records kept — targets earn what they get
// SIT Doom Squad escalation — special operations
// AiZQuaD — Undisputed_True 2025
// ═══════════════════════════════════════════════════════

namespace mesh {

// Forward declarations
class ERM;
class TokenFlow;
class FuelFlow;
class GateStats;

// ── CENSOR MODES ─────────────────────────────────────
enum class CensorMode : uint8_t {
    ADVISE    = 0,   // Warning only (was SOFT)
    REPRIMAND = 1,   // Logged reprimand (was FIRM)
    SUSPEND   = 2,   // Temporary suspension (was HARD)
    STRIKE    = 3,   // Formal strike
    STUN      = 4,   // Temporary incapacitation
    TERMINATE = 5,   // Permanent termination (was ZAP)
    PURGE     = 6    // Complete removal + blacklist (was NUKE)
};

// ── CENSOR RESULT ────────────────────────────────────
enum class CensorResult : uint8_t {
    CLEAR              = 0,   // No action taken (was OK)
    DENIED             = 1,   // Action blocked
    NO_TARGET          = 2,   // Target not found
    ALREADY_TERMINATED = 3,   // Target already terminated (was ALREADY_DEAD)
    ESCALATED          = 4,   // Escalated to SIT
    SELF_DEFENSE       = 5,   // Self-defense protocol engaged
    NO_WARRANT         = 6    // No authorization
};

// ── GREEN LIGHT STATE ────────────────────────────────
enum class GLState : uint8_t {
    ACTIVE     = 0,   // Ready to fire
    EXECUTED   = 1,   // Hit at least once
    RE_ENGAGED = 2,   // Target came back — hit again
    ESCALATED  = 3    // SIT notified — special operation
};

// ── INCIDENT TYPES ───────────────────────────────────
// For zero tolerance classification
enum class IncidentType : uint8_t {
    ROOM_FALL          = 0,
    ACCOUNT_TAKEOVER   = 1,
    HOSTILE_CONNECT    = 2,
    MESH_PROBE         = 3,
    TOKEN_FORGE        = 4,
    FUEL_DRAIN         = 5,
    REPLAY_ATTACK      = 6,
    OTHER              = 255
};

// ── WARRANT STRUCTURE ────────────────────────────────
struct Warrant {
    uint64_t warrant_id;
    uint64_t issuer_id;
    uint64_t target_id;
    CensorMode max_mode;
    uint64_t expiry_ms;
    bool valid;
};

// ── CASE RECORD ──────────────────────────────────────
// Every enforcement documented — 300 cases = 300 records
// Judge gets receipt of each
struct CensorRecord {
    uint64_t    case_id;                  // was zap_id
    std::string target_id;
    std::string warrant_id;
    std::string green_light_id;
    uint8_t     executing_sector;         // was executing_room
    uint8_t     enforcement_count;        // was zap_count
    CensorMode  mode;
    CensorResult result;
    GLState     gl_state;
    uint64_t    first_action_ms;          // was first_zap_ms
    uint64_t    last_action_ms;           // was last_zap_ms
    bool        executed;
    bool        escalated;
    bool        zero_tolerance;
    bool        judge_notified;
    char        reason[128];
    char        evidence[256];
};

// ── SIT OPERATION ────────────────────────────────────
// SIT Doom Squad — special operation
struct SITOperation {
    uint64_t    op_id;
    std::string target_id;
    uint64_t    trigger_case_id;          // was trigger_zap_id
    uint8_t     enforcement_count;        // was zap_count
    uint8_t     assigned_sector;          // was assigned_room
    uint64_t    opened_ms;
    bool        active;
    char        reason[128];
};

// ── CENSOR ENGINE ────────────────────────────────────
class Censor {
public:
    static constexpr uint8_t  MAX_CASE_RECORDS     = 128;  // was MAX_ZAP_RECORDS
    static constexpr uint8_t  MAX_SIT_OPS          = 32;
    static constexpr uint8_t  SIT_SECTOR           = 5;    // was SIT_ROOM
    static constexpr uint8_t  SECTOR_ZERO          = 0;    // was ROOM_ZERO
    static constexpr uint8_t  CASE_ESCALATE_AT     = 3;    // was ZAP_ESCALATE_AT

    Censor(uint8_t      sector_id,                         // was room_id
           ERM*         erm,
           TokenFlow*   token_flow_ptr,                    // was token_engine
           FuelFlow*    fuel_flow,
           GateStats*   gate_stats);

    ~Censor();

    // ── LIFECYCLE ────────────────────────────────────
    bool init(uint64_t now_ms);
    void shutdown();

    // ── ENFORCE ──────────────────────────────────────
    // Main fire function — needs warrant + green light
    CensorResult enforce(const std::string& target_id,     // was execute
                        CensorMode         mode,
                        const char*        reason,
                        uint64_t           now_ms);

    // Zero tolerance — self defense — no warrant needed
    CensorResult enforce_zero_tolerance(const std::string& target_id,  // was execute_zero_tolerance
                                       IncidentType       incident,
                                       const char*        evidence,
                                       uint64_t           now_ms);

    // Sector Zero only — cascade purge
    CensorResult purge(const std::string& target_id,       // was execute_nuke
                      const char*        reason,
                      uint64_t           now_ms);

    // ── RECORDS ──────────────────────────────────────
    // Get case record for target
    const CensorRecord* get_case(const std::string& target_id) const;  // was get_zap_record

    // Get total enforcement count for target
    uint8_t get_enforcement_count(const std::string& target_id) const; // was get_zap_count

    // Check if target is escalated to SIT
    bool is_escalated(const std::string& target_id) const;

    // ── JUDGE REPORTING ──────────────────────────────
    // Send case receipt to Judge
    bool report_to_judge(uint64_t case_id, uint64_t now_ms);  // was report_zap_to_judge

    // ── SIT ESCALATION ───────────────────────────────
    // Escalate repeat offender to SIT Doom Squad
    bool escalate_to_sit(const std::string& target_id,
                        uint64_t           case_id,
                        uint64_t           now_ms);

    // ── STATE ────────────────────────────────────────
    uint8_t get_sector_id()          const { return sector_id_; }       // was get_room_id
    uint8_t get_case_record_count()  const { return case_count_; }      // was get_zap_record_count
    uint8_t get_sit_op_count()       const { return sit_op_count_; }

    void print_status()    const;
    void print_case_log()  const;                                        // was print_zap_log

private:
    // ── INTERNAL ─────────────────────────────────────
    uint64_t       next_case_id();                                       // was next_zap_id
    uint64_t       next_op_id();
    CensorRecord*  find_or_create_case(const std::string& target_id,    // was find_or_create_record
                                      uint64_t           now_ms);

    // Execution steps
    bool strike_target(const std::string& target_id, uint64_t now_ms);
    bool stun_target  (const std::string& target_id, uint64_t now_ms);
    bool terminate_target(const std::string& target_id, uint64_t now_ms);  // was zap_target
    bool purge_target (const std::string& target_id, uint64_t now_ms);     // was nuke_target

    // Wipe target data
    void wipe_target_data(const std::string& target_id, uint64_t now_ms);

    uint8_t      sector_id_;                                // was room_id_
    ERM*         erm_;
    TokenFlow*   token_flow_ptr_;                           // was token_engine_
    FuelFlow*    fuel_flow_;
    GateStats*   gate_stats_;

    std::array<CensorRecord,  MAX_CASE_RECORDS> case_records_;  // was zap_records_
    std::array<SITOperation,  MAX_SIT_OPS>      sit_ops_;

    uint8_t  case_count_;                                   // was zap_count_
    uint8_t  sit_op_count_;
    uint64_t case_id_counter_;                              // was zap_id_counter_
    uint64_t op_id_counter_;
    uint64_t init_ms_;
};

} // namespace mesh

#endif // FC_FLOW_CENSOR_HPP
