#include "../src/burner/cross_validator.hpp"
#include <iostream>
#include <openssl/rand.h>

using namespace fc_flow::burner;

int main() {
    std::cout << "\n╔════════════════════════════════════╗\n";
    std::cout <<   "║   CROSS-VALIDATOR TEST             ║\n";
    std::cout <<   "╚════════════════════════════════════╝\n\n";
    
    CrossValidator validator;
    
    // Generate seeds
    uint8_t undisputed_seed[32];
    uint8_t flex_secret[32];
    RAND_bytes(undisputed_seed, 32);
    RAND_bytes(flex_secret, 32);
    
    std::cout << "Seeds generated\n\n";
    
    // ── TEST 1: Valid token ──
    std::cout << "TEST 1 — Valid Token:\n";
    auto token = validator.mint(
        12345,                  // cycle_id
        3,                      // room_id
        undisputed_seed,
        flex_secret,
        "test_entropy",
        "test_tick_salt"
    );
    
    bool valid = validator.validate(
        token,
        undisputed_seed,
        flex_secret,
        "test_entropy",
        "test_tick_salt"
    );
    
    std::cout << "Result: " << (valid ? "PASS" : "FAIL") << "\n\n";
    
    // ── TEST 2: Invalid token (wrong tick_salt) ──
    std::cout << "TEST 2 — Invalid Token (wrong tick_salt):\n";
    bool invalid = validator.validate(
        token,
        undisputed_seed,
        flex_secret,
        "test_entropy",
        "WRONG_tick_salt"  // Changed
    );
    
    std::cout << "Result: " << (invalid ? "FAIL (should reject)" : "PASS") << "\n\n";
    
    // ── TEST 3: Dissolve to bait ──
    std::cout << "TEST 3 — Dissolve to Bait:\n";
    uint8_t temp_seed[32];
    uint8_t temp_secret[32];
    std::copy(undisputed_seed, undisputed_seed + 32, temp_seed);
    std::copy(flex_secret, flex_secret + 32, temp_secret);
    
    validator.dissolve_to_bait(token, temp_seed, temp_secret);
    
    std::cout << "Token dissolved: " << !token.is_valid() << "\n";
    std::cout << "Undisputed alive: " << token.undisputed.alive << "\n";
    std::cout << "Flex alive: " << token.flex.alive << "\n\n";
    
    // ── TEST 4: Honeypot detection ──
    std::cout << "TEST 4 — Honeypot Detection:\n";
    bool is_honeypot = validator.check_honeypot_use(
        token.undisputed.live_hash  // The bait
    );
    
    std::cout << "Honeypot triggered: " << is_honeypot << "\n";
    std::cout << "Room 5 queue size: " << 
        validator.get_intake_queue().size() << "\n\n";
    
    std::cout << "╔════════════════════════════════════╗\n";
    std::cout << "║   ALL TESTS COMPLETE               ║\n";
    std::cout << "╚════════════════════════════════════╝\n\n";
    
    return 0;
}
