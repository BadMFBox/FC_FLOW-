#include "fc_flow/enforcement/burner_bridge.hpp"
#include <cstring>

namespace fc_flow::enforcement {

BurnerBridge::BurnerBridge(Monitus& monitus) 
    : monitus_(monitus)
    , gate_(std::make_unique<burner::PredatorGate>(true))
{
    sector_has_token_.fill(false);
}

void BurnerBridge::on_bolt_fired(uint8_t sector_id, uint64_t flow_cycle) noexcept {
    // Token successfully consumed, clear cached token
    sector_has_token_[sector_id] = false;
}

void BurnerBridge::on_bolt_failed(uint8_t sector_id, uint64_t flow_cycle) noexcept {
    // Monitus already recorded the failure
    // PredatorGate will increment strikes on next token operation
    sector_has_token_[sector_id] = false;
}

bool BurnerBridge::verify_with_token(uint8_t sector_id, uint64_t flow_cycle) noexcept {
    // Birth a new token for this validation cycle
    auto birth_result = gate_->birth_token(sector_id);
    
    if (!birth_result.success) {
        // Birth failed (rate limit, no fuel, or burned)
        monitus_.witness_bolt_failed(sector_id, flow_cycle, MonitusEventType::BOLT_FAILED);
        return false;
    }
    
    // Cache the token
    std::memcpy(sector_tokens_[sector_id].data(), 
                birth_result.token_value->data(), 32);
    sector_has_token_[sector_id] = true;
    
    // Immediately admit it (consume)
    auto admit_result = gate_->admit(sector_id, sector_tokens_[sector_id].data());
    
    if (!admit_result.success) {
        // Admission failed (wrong sector, expired, bad HMAC, etc.)
        monitus_.witness_bolt_failed(sector_id, flow_cycle, MonitusEventType::BOLT_FAILED);
        sector_has_token_[sector_id] = false;
        return false;
    }
    
    // Success — token consumed
    sector_has_token_[sector_id] = false;
    return true;
}

const burner::GateStats& BurnerBridge::get_gate_stats() const noexcept {
    return gate_->get_stats();
}

bool BurnerBridge::is_burning() const noexcept {
    return gate_->is_burning();
}

} // namespace fc_flow::enforcement
