#pragma once
#include <cstdint>
#include <string>

namespace mesh {

struct Warrant;

class ERM {
public:
    bool has_warrant(const std::string& target_id, uint64_t now_ms) const { return false; }
    bool has_green_light(const std::string& target_id, uint64_t now_ms) const { return false; }
    const Warrant* get_warrant(const std::string& target_id) const { return nullptr; }
    void mark_executed(const std::string& target_id, uint64_t now_ms) {}
    void command_room(uint8_t room, uint8_t cmd, uint64_t now_ms) {}
};

class TokenFlow {
public:
    void invalidate(uint32_t target_id) {}
};

class FuelFlow {
public:
    void throttle(uint32_t target_id, uint64_t now_ms) {}
};

class GateStats {
public:
    void record_hit(uint8_t sector_id, uint64_t now_ms) {}
};

} // namespace mesh
