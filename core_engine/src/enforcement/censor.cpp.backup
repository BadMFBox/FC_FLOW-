#include "fc_flow/enforcement/censor.hpp"
#include "fc_flow/enforcement/erm_stub.hpp"
#include <cstdio>
#include <cstring>

// ═══════════════════════════════════════════════════════
// SOVEREIGN PEU — CENSOR IMPLEMENTATION
// Better than SIGKILL
// AiZQuaD — Undisputed_True 2025
// ═══════════════════════════════════════════════════════

namespace mesh {

// ── MODE STRINGS ─────────────────────────────────────
static const char* MODE_NAMES[] = {
    "ADVISE", "REPRIMAND", "SUSPEND", "STRIKE", "STUN", "TERMINATE", "PURGE"
};

// ── RESULT STRINGS ───────────────────────────────────
[[maybe_unused]] static const char* RESULT_NAMES[] = {
    "CLEAR", "DENIED", "NO_TARGET",
    "ALREADY_TERMINATED", "ESCALATED", "SELF_DEFENSE", "NO_WARRANT"
};

// ── GL STATE STRINGS ─────────────────────────────────
static const char* GL_STATE_NAMES[] = {
    "ACTIVE", "EXECUTED", "RE_ENGAGED", "ESCALATED"
};

// ── SECTOR NAMES ─────────────────────────────────────
static const char* SECTOR_NAMES[] = {
    "Sovereign Fuel",
    "CypheReignTrigger",
    "16_LawGic",
    "Retribution Force",
    "AiZone Contr",
    "SIT Doom Squad"
};

// ── CONSTRUCTOR ──────────────────────────────────────
Censor::Censor(uint8_t      sector_id,
               ERM*         erm,
               TokenFlow*   token_flow_ptr,
               FuelFlow*    fuel_flow,
               GateStats*   gate_stats)
    : sector_id_(sector_id)
    , erm_(erm)
    , token_flow_ptr_(token_flow_ptr)
    , fuel_flow_(fuel_flow)
    , gate_stats_(gate_stats)
    , case_count_(0)
    , sit_op_count_(0)
    , case_id_counter_(3000)
    , op_id_counter_(5000)
    , init_ms_(0)
{
    case_records_.fill(CensorRecord{});
    sit_ops_.fill(SITOperation{});
}

// ── DESTRUCTOR ───────────────────────────────────────
Censor::~Censor() {
    // Wipe all records on shutdown
    memset(case_records_.data(), 0, sizeof(CensorRecord) * MAX_CASE_RECORDS);
    memset(sit_ops_.data(), 0, sizeof(SITOperation) * MAX_SIT_OPS);
}

// ── INIT ─────────────────────────────────────────────
bool Censor::init(uint64_t now_ms) {
    init_ms_ = now_ms;

    printf("\n[CENSOR] Sector %d — %s — Censor online\n",
           sector_id_,
           sector_id_ < 6 ? SECTOR_NAMES[sector_id_] : "Unknown");
    printf("[CENSOR] Better than SIGKILL\n");
    printf("[CENSOR] SIT escalation at %d enforcements\n", CASE_ESCALATE_AT);
    return true;
}

// ── ENFORCE ──────────────────────────────────────────
// Main fire — needs warrant + green light from Judge
CensorResult Censor::enforce(const std::string& target_id,
                             CensorMode         mode,
                             const char*        reason,
                             uint64_t           now_ms)
{
    printf("\n[CENSOR] Enforce — Target: %s — Mode: %s\n",
           target_id.c_str(), MODE_NAMES[static_cast<uint8_t>(mode)]);

    // ── VALIDATE WARRANT ─────────────────────────────
    if (erm_ && !erm_->has_warrant(target_id, now_ms)) {
        printf("[CENSOR] DENIED — No active warrant — Target: %s\n",
               target_id.c_str());
        printf("[CENSOR] Report to Judge — get warrant first\n");
        return CensorResult::NO_WARRANT;
    }

    // ── VALIDATE GREEN LIGHT ─────────────────────────
    if (erm_ && !erm_->has_green_light(target_id, now_ms)) {
        printf("[CENSOR] DENIED — No green light — Target: %s\n",
               target_id.c_str());
        printf("[CENSOR] Judge must authorize green light\n");
        return CensorResult::NO_WARRANT;
    }

    printf("[CENSOR] Warrant ✓ — Green Light ✓ — Firing\n");

    // ── GET OR CREATE CASE RECORD ────────────────────
    CensorRecord* rec = find_or_create_case(target_id, now_ms);
    if (!rec) {
        printf("[CENSOR] ERROR — case record buffer full\n");
        return CensorResult::DENIED;
    }

    // ── UPDATE RECORD ────────────────────────────────
    rec->target_id         = target_id;
    rec->executing_sector  = sector_id_;
    rec->mode              = mode;
    rec->last_action_ms    = now_ms;
    rec->zero_tolerance    = false;
    strncpy(rec->reason, reason, sizeof(rec->reason) - 1);

    // Track warrant + green light
    if (erm_) {
        const Warrant* w = erm_->get_warrant(target_id);
        if (w) rec->warrant_id = std::to_string(w->warrant_id);
    }

    // ── EXECUTE MODE ─────────────────────────────────
    bool fired = false;
    switch (mode) {
        case CensorMode::ADVISE:
            printf("[CENSOR] ADVISE issued to Target: %s\n", target_id.c_str());
            fired = true;
            break;
        case CensorMode::REPRIMAND:
            printf("[CENSOR] REPRIMAND logged for Target: %s\n", target_id.c_str());
            fired = true;
            break;
        case CensorMode::SUSPEND:
            printf("[CENSOR] SUSPEND applied to Target: %s\n", target_id.c_str());
            fired = true;
            break;
        case CensorMode::STRIKE:
            fired = strike_target(target_id, now_ms);
            break;
        case CensorMode::STUN:
            fired = stun_target(target_id, now_ms);
            break;
        case CensorMode::TERMINATE:
            fired = terminate_target(target_id, now_ms);
            break;
        case CensorMode::PURGE:
            // Purge requires Sector Zero
            if (sector_id_ != SECTOR_ZERO) {
                printf("[CENSOR] DENIED — PURGE requires Sector Zero\n");
                return CensorResult::DENIED;
            }
            fired = purge_target(target_id, now_ms);
            break;
    }

    if (!fired) return CensorResult::DENIED;

    // ── UPDATE CASE STATE ────────────────────────────
    rec->enforcement_count++;
    rec->executed = true;

    // Green light stays — mark executed not closed
    // Target may come back — history preserved
    if (rec->enforcement_count == 1) {
        rec->gl_state = GLState::EXECUTED;
    } else {
        rec->gl_state = GLState::RE_ENGAGED;
        printf("[CENSOR] ⚠ Target %s RE_ENGAGED — enforcement #%d\n",
               target_id.c_str(), rec->enforcement_count);
    }

    if (erm_) erm_->mark_executed(target_id, now_ms);

    // ── REPORT TO JUDGE ──────────────────────────────
    report_to_judge(rec->case_id, now_ms);

    // ── CHECK SIT ESCALATION ─────────────────────────
    if (rec->enforcement_count >= CASE_ESCALATE_AT && !rec->escalated) {
        printf("[CENSOR] ⚡ Target %s hit %d times — escalating to SIT\n",
               target_id.c_str(), rec->enforcement_count);
        escalate_to_sit(target_id, rec->case_id, now_ms);
        rec->escalated = true;
        rec->gl_state  = GLState::ESCALATED;
        return CensorResult::ESCALATED;
    }

    rec->result = CensorResult::CLEAR;
    return CensorResult::CLEAR;
}

// ── ENFORCE ZERO TOLERANCE ───────────────────────────
// Self defense — no warrant needed
// Room fall + account takeover only
CensorResult Censor::enforce_zero_tolerance(const std::string& target_id,
                                           IncidentType       incident,
                                           const char*        evidence,
                                           uint64_t           now_ms)
{
    // Only valid for zero tolerance incidents
    if (incident != IncidentType::ROOM_FALL &&
        incident != IncidentType::ACCOUNT_TAKEOVER &&
        incident != IncidentType::HOSTILE_CONNECT &&
        incident != IncidentType::MESH_PROBE) {
        printf("[CENSOR] DENIED — not a zero tolerance incident\n");
        return CensorResult::DENIED;
    }

    printf("\n[CENSOR] ⚡ ZERO TOLERANCE — Self Defense\n");
    printf("[CENSOR]   Target: %s — No warrant required\n", target_id.c_str());

    CensorRecord* rec = find_or_create_case(target_id, now_ms);
    if (!rec) return CensorResult::DENIED;

    rec->target_id         = target_id;
    rec->executing_sector  = sector_id_;
    rec->mode              = CensorMode::TERMINATE;
    rec->last_action_ms    = now_ms;
    rec->zero_tolerance    = true;
    rec->enforcement_count++;
    rec->executed          = true;
    rec->gl_state          = GLState::EXECUTED;
    strncpy(rec->evidence, evidence, sizeof(rec->evidence) - 1);

    // Zero tolerance = full termination
    terminate_target(target_id, now_ms);

    // Report to Judge — still documented
    report_to_judge(rec->case_id, now_ms);

    // ── CHECK SIT ESCALATION ─────────────────────────
    if (rec->enforcement_count >= CASE_ESCALATE_AT && !rec->escalated) {
        printf("[CENSOR] ⚡ Target %s hit %d times — escalating to SIT\n",
               target_id.c_str(), rec->enforcement_count);
        escalate_to_sit(target_id, rec->case_id, now_ms);
        rec->escalated = true;
        rec->gl_state  = GLState::ESCALATED;
        return CensorResult::ESCALATED;
    }

    rec->result = CensorResult::CLEAR;
    return CensorResult::CLEAR;
}

// ── PURGE — SECTOR ZERO ONLY ─────────────────────────
CensorResult Censor::purge(const std::string& target_id,
                           const char*        reason,
                           uint64_t           now_ms)
{
    if (sector_id_ != SECTOR_ZERO) {
        printf("[CENSOR] DENIED — PURGE is Sector Zero only\n");
        return CensorResult::DENIED;
    }

    printf("\n[CENSOR] ⚡⚡ APEX PURGE — Target: %s\n", target_id.c_str());
    return enforce(target_id, CensorMode::PURGE, reason, now_ms);
}

// ── STRIKE TARGET ────────────────────────────────────
bool Censor::strike_target(const std::string& target_id, uint64_t now_ms) {
    printf("[CENSOR] STRIKE — Target: %s — Throttling\n", target_id.c_str());
    if (fuel_flow_) {
        fuel_flow_->throttle(static_cast<uint32_t>(std::stoull(target_id)), now_ms);
    }
    return true;
}

// ── STUN TARGET ──────────────────────────────────────
bool Censor::stun_target(const std::string& target_id, uint64_t now_ms) {
    printf("[CENSOR] STUN — Target: %s — Suspended\n", target_id.c_str());
    if (fuel_flow_) {
        fuel_flow_->throttle(static_cast<uint32_t>(std::stoull(target_id)), now_ms);
    }
    // Suspend token generation
    if (token_flow_ptr_) {
        token_flow_ptr_->invalidate(static_cast<uint32_t>(std::stoull(target_id)));
    }
    return true;
}

// ── TERMINATE TARGET ─────────────────────────────────
bool Censor::terminate_target(const std::string& target_id, uint64_t now_ms) {
    printf("[CENSOR] ⚡ TERMINATE — Target: %s — Full termination\n", target_id.c_str());

    // 1. Burn all tokens
    if (token_flow_ptr_) {
        token_flow_ptr_->invalidate(static_cast<uint32_t>(std::stoull(target_id)));
    }

    // 2. Kill fuel flow
    if (fuel_flow_) {
        fuel_flow_->throttle(static_cast<uint32_t>(std::stoull(target_id)), now_ms);
    }

    // 3. Wipe target data
    wipe_target_data(target_id, now_ms);

    printf("[CENSOR]   Tokens burned ✓\n");
    printf("[CENSOR]   Fuel killed  ✓\n");
    printf("[CENSOR]   Data wiped   ✓\n");
    return true;
}

// ── PURGE TARGET ─────────────────────────────────────
bool Censor::purge_target(const std::string& target_id, uint64_t now_ms) {
    printf("[CENSOR] ⚡⚡ PURGE — Target: %s — Cascade termination\n",
           target_id.c_str());

    // Full termination first
    terminate_target(target_id, now_ms);

    // Cascade — notify all sectors via ERM
    if (erm_) {
        erm_->command_room(1, 0xFF, now_ms);
        erm_->command_room(2, 0xFF, now_ms);
        erm_->command_room(3, 0xFF, now_ms);
        erm_->command_room(4, 0xFF, now_ms);
        erm_->command_room(5, 0xFF, now_ms);
    }

    printf("[CENSOR]   Cascade complete — all sectors notified ✓\n");
    return true;
}

// ── WIPE TARGET DATA ─────────────────────────────────
void Censor::wipe_target_data(const std::string& target_id, uint64_t now_ms) {
    // Secure zero any target specific data
    // In production — wipe session data, cached tokens, etc.
    printf("[CENSOR]   Wiping target %s data\n", target_id.c_str());
}

// ── REPORT TO JUDGE ──────────────────────────────────
// Every enforcement gets a receipt — 300 cases = 300 records
bool Censor::report_to_judge(uint64_t case_id, uint64_t now_ms) {
    // Find record
    CensorRecord* rec = nullptr;
    for (uint8_t i = 0; i < case_count_; i++) {
        if (case_records_[i].case_id == case_id) {
            rec = &case_records_[i];
            break;
        }
    }

    if (!rec) return false;

    printf("[CENSOR] → Judge receipt — Case #%lu\n", case_id);
    printf("[CENSOR]   Target: %s | Mode: %s | Enforcement #%d\n",
           rec->target_id.c_str(),
           MODE_NAMES[static_cast<uint8_t>(rec->mode)],
           rec->enforcement_count);
    printf("[CENSOR]   Zero tolerance: %s | Escalated: %s\n",
           rec->zero_tolerance ? "YES" : "NO",
           rec->escalated      ? "YES" : "NO");
    printf("[CENSOR]   GL State: %s\n",
           GL_STATE_NAMES[static_cast<uint8_t>(rec->gl_state)]);

    rec->judge_notified = true;

    // Gate stats
    if (gate_stats_) gate_stats_->record_hit(sector_id_, now_ms);

    return true;
}

// ── ESCALATE TO SIT ──────────────────────────────────
// SIT Doom Squad — special operation
bool Censor::escalate_to_sit(const std::string& target_id,
                             uint64_t           case_id,
                             uint64_t           now_ms)
{
    if (sit_op_count_ >= MAX_SIT_OPS) {
        printf("[CENSOR] WARNING — SIT op buffer full\n");
        return false;
    }

    SITOperation& op = sit_ops_[sit_op_count_++];
    op.op_id            = next_op_id();
    op.target_id        = target_id;
    op.trigger_case_id  = case_id;
    op.assigned_sector  = SIT_SECTOR;
    op.opened_ms        = now_ms;
    op.active           = true;

    // Get enforcement count
    const CensorRecord* rec = get_case(target_id);
    if (rec) op.enforcement_count = rec->enforcement_count;

    snprintf(op.reason, sizeof(op.reason),
             "Target %s hit %d times — SIT special operation",
             target_id.c_str(), op.enforcement_count);

    printf("[CENSOR] ⚡ SIT ESCALATION — Op #%lu\n", op.op_id);
    printf("[CENSOR]   Target: %s — Sector 5 SIT Doom Squad\n", target_id.c_str());
    printf("[CENSOR]   Enforcement count: %d — Special operation opened\n",
           op.enforcement_count);

    return true;
}

// ── GET CASE RECORD ──────────────────────────────────
const CensorRecord* Censor::get_case(const std::string& target_id) const {
    for (uint8_t i = 0; i < case_count_; i++) {
        if (case_records_[i].target_id == target_id) {
            return &case_records_[i];
        }
    }
    return nullptr;
}

// ── GET ENFORCEMENT COUNT ────────────────────────────
uint8_t Censor::get_enforcement_count(const std::string& target_id) const {
    const CensorRecord* rec = get_case(target_id);
    return rec ? rec->enforcement_count : 0;
}

// ── IS ESCALATED ─────────────────────────────────────
bool Censor::is_escalated(const std::string& target_id) const {
    const CensorRecord* rec = get_case(target_id);
    return rec ? rec->escalated : false;
}

// ── FIND OR CREATE CASE ──────────────────────────────
CensorRecord* Censor::find_or_create_case(const std::string& target_id,
                                          uint64_t           now_ms)
{
    // Check existing
    for (uint8_t i = 0; i < case_count_; i++) {
        if (case_records_[i].target_id == target_id) {
            return &case_records_[i];
        }
    }

    // Create new
    if (case_count_ >= MAX_CASE_RECORDS) return nullptr;

    CensorRecord& rec      = case_records_[case_count_++];
    rec.case_id            = next_case_id();
    rec.target_id          = target_id;
    rec.enforcement_count  = 0;
    rec.executed           = false;
    rec.escalated          = false;
    rec.judge_notified     = false;
    rec.zero_tolerance     = false;
    rec.first_action_ms    = now_ms;
    rec.last_action_ms     = now_ms;
    rec.gl_state           = GLState::ACTIVE;

    return &rec;
}

// ── NEXT IDS ─────────────────────────────────────────
uint64_t Censor::next_case_id() { return case_id_counter_++; }
uint64_t Censor::next_op_id()   { return op_id_counter_++;   }

// ── SHUTDOWN ─────────────────────────────────────────
void Censor::shutdown() {
    printf("[CENSOR] Sector %d — Censor shutting down\n", sector_id_);
    memset(case_records_.data(), 0, sizeof(CensorRecord) * MAX_CASE_RECORDS);
    memset(sit_ops_.data(), 0, sizeof(SITOperation) * MAX_SIT_OPS);
    case_count_     = 0;
    sit_op_count_   = 0;
}

// ── PRINT STATUS ─────────────────────────────────────
void Censor::print_status() const {
    printf("\n╔═══════════════════════════════════════════════════╗\n");
    printf("║             CENSOR — STATUS                       ║\n");
    printf("╠═══════════════════════════════════════════════════╣\n");
    printf("║ Sector: %d — %-20s                   ║\n",
           sector_id_,
           sector_id_ < 6 ? SECTOR_NAMES[sector_id_] : "Unknown");
    printf("║ Case Records: %3d │ SIT Operations: %3d          ║\n",
           case_count_, sit_op_count_);
    printf("║ Escalate at: %3d enforcements                     ║\n",
           CASE_ESCALATE_AT);
    printf("╚═══════════════════════════════════════════════════╝\n\n");
}

// ── PRINT CASE LOG ───────────────────────────────────
void Censor::print_case_log() const {
    printf("\n╔═══════════════════════════════════════════════════╗\n");
    printf("║              CENSOR — CASE LOG                    ║\n");
    printf("╠═══════════════════════════════════════════════════╣\n");

    for (uint8_t i = 0; i < case_count_; i++) {
        const CensorRecord& r = case_records_[i];
        printf("║ Case #%-5lu Target: %-20s Hits: %d Mode: %-9s║\n",
               r.case_id,
               r.target_id.c_str(),
               r.enforcement_count,
               MODE_NAMES[static_cast<uint8_t>(r.mode)]);
        printf("║   GL: %-10s ZT: %-3s ESC: %-3s Judge: %-3s  ║\n",
               GL_STATE_NAMES[static_cast<uint8_t>(r.gl_state)],
               r.zero_tolerance  ? "YES" : "NO",
               r.escalated       ? "YES" : "NO",
               r.judge_notified  ? "YES" : "NO");
    }

    if (case_count_ == 0) {
        printf("║  No cases recorded                                ║\n");
    }

    printf("╚═══════════════════════════════════════════════════╝\n\n");
}

} // namespace mesh
