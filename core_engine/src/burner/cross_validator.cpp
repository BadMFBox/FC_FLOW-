#include "cross_validator.hpp"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace fc_flow::burner {

CrossValidator::CrossValidator() {
    log("CROSS-VALIDATOR INITIALIZED");
}

CrossValidator::~CrossValidator() {
    log("CROSS-VALIDATOR SHUTDOWN");
}

// ── MINT TOKEN (Both Calculated) ──────────────────────

CrossValidatedToken CrossValidator::mint(
    uint64_t cycle_id,
    uint8_t room_id,
    const uint8_t* undisputed_seed,
    const uint8_t* flex_secret,
    const std::string& cycle_entropy,
    const std::string& tick_salt
) {
    CrossValidatedToken token{};
    
    auto now = std::chrono::system_clock::now();
    token.cycle_id = cycle_id;
    token.source_room = room_id;
    token.target_room = (room_id + 1) % 6;
    token.issued_at = std::chrono::duration_cast<
        std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    token.expires_at = token.issued_at + 270;  // 0.27 sec
    
    // ── UNDISPUTED (PIN_A): CALCULATE from seed ──
    std::string undisputed_data =
        std::string(reinterpret_cast<const char*>(undisputed_seed), 32) +
        "||" + std::to_string(cycle_id) +
        "||" + std::to_string(room_id) +
        "||" + cycle_entropy +
        "||" + tick_salt;
    
    unsigned char undisputed_hmac[32];
    unsigned int hmac_len = 32;
    HMAC(
        EVP_sha256(),
        undisputed_seed, 32,
        reinterpret_cast<const unsigned char*>(undisputed_data.c_str()),
        undisputed_data.size(),
        undisputed_hmac,
        &hmac_len
    );
    
    std::copy(undisputed_hmac, undisputed_hmac + 32,
              token.undisputed.live_hash.begin());
    token.undisputed.factor_one = true;
    token.undisputed.alive = true;
    
    // ── FLEX (PIN_B): CALCULATE from flex_secret ──
    std::string flex_data =
        std::string(reinterpret_cast<const char*>(flex_secret), 32) +
        "||" + std::to_string(room_id);
    
    unsigned char flex_hmac[32];
    HMAC(
        EVP_sha256(),
        flex_secret, 32,
        reinterpret_cast<const unsigned char*>(flex_data.c_str()),
        flex_data.size(),
        flex_hmac,
        &hmac_len
    );
    
    std::copy(flex_hmac, flex_hmac + 32,
              token.flex.flex_hash.begin());
    token.flex.factor_two = true;
    token.flex.alive = true;
    
    log("TOKEN MINTED — cycle " + std::to_string(cycle_id) +
        " room " + std::to_string(room_id));
    
    return token;
}

// ── VALIDATE TOKEN (Cross-Check Both) ─────────────────

bool CrossValidator::validate(
    const CrossValidatedToken& token,
    const uint8_t* undisputed_seed,
    const uint8_t* flex_secret,
    const std::string& cycle_entropy,
    const std::string& tick_salt
) {
    // ── CHECK UNDISPUTED (PIN_A) ──
    std::string undisputed_data =
        std::string(reinterpret_cast<const char*>(undisputed_seed), 32) +
        "||" + std::to_string(token.cycle_id) +
        "||" + std::to_string(token.source_room) +
        "||" + cycle_entropy +
        "||" + tick_salt;
    
    unsigned char expected_undisputed[32];
    unsigned int hmac_len = 32;
    HMAC(
        EVP_sha256(),
        undisputed_seed, 32,
        reinterpret_cast<const unsigned char*>(undisputed_data.c_str()),
        undisputed_data.size(),
        expected_undisputed,
        &hmac_len
    );
    
    bool undisputed_valid = std::equal(
        expected_undisputed, expected_undisputed + 32,
        token.undisputed.live_hash.begin()
    );
    
    // ── CHECK FLEX (PIN_B) ──
    std::string flex_data =
        std::string(reinterpret_cast<const char*>(flex_secret), 32) +
        "||" + std::to_string(token.source_room);
    
    unsigned char expected_flex[32];
    HMAC(
        EVP_sha256(),
        flex_secret, 32,
        reinterpret_cast<const unsigned char*>(flex_data.c_str()),
        flex_data.size(),
        expected_flex,
        &hmac_len
    );
    
    bool flex_valid = std::equal(
        expected_flex, expected_flex + 32,
        token.flex.flex_hash.begin()
    );
    
    // ── CROSS-VALIDATION ──
    bool both_valid = undisputed_valid && flex_valid;
    
    if (!both_valid) {
        log("VALIDATION FAILED — cross-check failed");
        log("Undisputed: " + std::string(undisputed_valid ? "PASS" : "FAIL"));
        log("Flex: " + std::string(flex_valid ? "PASS" : "FAIL"));
        return false;
    }
    
    log("VALIDATION PASSED — both factors valid");
    return true;
}

// ── DISSOLVE TO BAIT (Both Die, Master Key Lives) ─────

void CrossValidator::dissolve_to_bait(
    CrossValidatedToken& token,
    uint8_t* undisputed_seed,
    uint8_t* flex_secret
) {
    log("AUTH FAIL — DISSOLVING TO BAIT");
    
    // ── 1) SAVE MASTER KEY (Undisputed hash) ──
    std::array<uint8_t, 32> master_key_bait = token.undisputed.live_hash;
    
    // ── 2) DISSOLVE UNDISPUTED SEED ──
    RAND_bytes(undisputed_seed, 32);  // Overwrite
    std::fill_n(undisputed_seed, 32, 0);  // Zero
    
    token.undisputed.alive = false;
    token.undisputed.factor_one = false;
    // live_hash stays intact — that's the bait
    
    // ── 3) DISSOLVE FLEX SECRET ──
    RAND_bytes(flex_secret, 32);
    std::fill_n(flex_secret, 32, 0);
    
    token.flex.alive = false;
    token.flex.factor_two = false;
    std::fill(token.flex.flex_hash.begin(),
              token.flex.flex_hash.end(), 0);
    
    // ── 4) REGISTER MASTER KEY AS HONEYPOT ──
    register_master_key_bait(master_key_bait);
    
    log("SEEDS DISSOLVED — real access GONE");
    log("MASTER KEY ACTIVE AS BAIT");
}

// ── REGISTER HONEYPOT ──────────────────────────────────

void CrossValidator::register_master_key_bait(
    const std::array<uint8_t, 32>& bait_hash
) {
    std::lock_guard<std::mutex> lock(honeypot_lock_);
    active_honeypots_.insert(bait_hash);
    
    std::ostringstream oss;
    for (auto byte : bait_hash) {
        oss << std::hex << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(byte);
    }
    
    log("HONEYPOT ARMED: " + oss.str());
}

// ── CHECK HONEYPOT USE ─────────────────────────────────

bool CrossValidator::check_honeypot_use(
    const std::array<uint8_t, 32>& presented_hash
) {
    std::lock_guard<std::mutex> lock(honeypot_lock_);
    
    if (active_honeypots_.count(presented_hash) > 0) {
        log("!!! HONEYPOT TRIGGERED !!!");
        route_to_room_5_intake(presented_hash);
        return true;
    }
    
    return false;
}

// ── ROUTE TO ROOM 5 ────────────────────────────────────

void CrossValidator::route_to_room_5_intake(
    const std::array<uint8_t, 32>& stolen_key
) {
    Room5Intake intake;
    intake.timestamp = std::chrono::system_clock::now();
    intake.stolen_key = stolen_key;
    intake.status = "CHECK_IN";
    intake.next_step = "SAND_BOX";
    intake.attacker_id = "UNKNOWN";  // Will be fingerprinted
    
    room_5_queue_.push(intake);
    
    log("CHECK-IN TO ROOM 5");
    log("Attacker routed to sand box");
}

// ── LOGGING ────────────────────────────────────────────

void CrossValidator::log(const std::string& msg) {
    std::cout << "[CROSS-VALIDATOR] " << msg << "\n";
}

} // namespace fc_flow::burner
