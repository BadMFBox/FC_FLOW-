#pragma once
#include <cstddef>

namespace fc_flow::burner {

class RAMWiper {
public:
    RAMWiper() = default;
    ~RAMWiper() noexcept;
    
    bool mlock_region(void* addr, size_t len) noexcept;
    bool munlock_region(void* addr, size_t len) noexcept;
    bool mlock_all() noexcept;
    bool munlock_all() noexcept;
    
    void secure_zero(void* ptr, size_t len) noexcept;
    void wipe_and_unlock(void* ptr, size_t len) noexcept;
    void flush_caches() noexcept;
    
private:
    bool all_locked_{false};
};

} // namespace fc_flow::burner
