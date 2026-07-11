#pragma once

#include "fc_flow/burner/cross_validated_token.hpp"
#include <mutex>
#include <set>
#include <queue>
#include <string>

namespace fc_flow::burner {

class CrossValidator {
public:
    CrossValidator();
    ~CrossValidator();
    
    // Mint token (both Undisputed + Flex calculated)
    CrossValidatedToken mint(
        uint64_t cycle_id,
        uint8_t room_id,
        const uint8_t* undisputed_seed,
        const uint8_t* flex_secret,
        const std::string& cycle_entropy,
        const std::string& tick_salt
    );
    
    // Validate token (cross-check both)
    bool validate(
        const CrossValidatedToken& token,
        const uint8_t* undisputed_seed,
        const uint8_t* flex_secret,
        const std::string& cycle_entropy,
        const std::string& tick_salt
    );
    
    // Dissolve on auth-fail (both die, Master Key lives as bait)
    void dissolve_to_bait(
        CrossValidatedToken& token,
        uint8_t* undisputed_seed,
        uint8_t* flex_secret
    );
    
    // Check if presented hash is honeypot
    bool check_honeypot_use(
        const std::array<uint8_t, 32>& presented_hash
    );
    
    // Get Room 5 intake queue
    std::queue<Room5Intake>& get_intake_queue() {
        return room_5_queue_;
    }

private:
    std::mutex honeypot_lock_;
    std::set<std::array<uint8_t, 32>> active_honeypots_;
    std::queue<Room5Intake> room_5_queue_;
    
    void register_master_key_bait(
        const std::array<uint8_t, 32>& bait_hash
    );
    
    void route_to_room_5_intake(
        const std::array<uint8_t, 32>& stolen_key
    );
    
    void log(const std::string& msg);
};

} // namespace fc_flow::burner
