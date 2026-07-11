#pragma once
#include "fc_flow/enforcement/monitus.hpp"
#include "fc_flow/burner/predator_gate.hpp"
#include <memory>

namespace fc_flow::enforcement {

class BurnerBridge {
public:
    explicit BurnerBridge(Monitus& monitus);
    
    // Called on bolt fire success (issue token for next validation)
    void on_bolt_fired(uint8_t sector_id, uint64_t flow_cycle) noexcept;
    
    // Called on bolt fire failure (increment strikes, check burn threshold)
    void on_bolt_failed(uint8_t sector_id, uint64_t flow_cycle) noexcept;
    
    // Called during cross-validation (birth + admit token)
    bool verify_with_token(uint8_t sector_id, uint64_t flow_cycle) noexcept;
    
    // Stats access
    const burner::GateStats& get_gate_stats() const noexcept;
    bool is_burning() const noexcept;
    
private:
    Monitus& monitus_;
    std::unique_ptr<burner::PredatorGate> gate_;
    
    // Cache for current tokens per sector
    std::array<std::array<uint8_t, 32>, 256> sector_tokens_;
    std::array<bool, 256> sector_has_token_;
};

} // namespace fc_flow::enforcement
