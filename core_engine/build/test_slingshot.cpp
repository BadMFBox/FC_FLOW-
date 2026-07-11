#include "burner/slingshot_validator.hpp"
#include "fc_flow/burner/token.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace fc_flow::burner;

int main() {
    std::cout << "\n╔════════════════════════════════════╗\n";
    std::cout <<   "║   SLINGSHOT VALIDATOR TEST         ║\n";
    std::cout <<   "╚════════════════════════════════════╝\n\n";
    
    SlingshotValidator sling(4);  // Use core 4
    sling.ignite();
    
    // Queue 300 tokens
    for (int i = 0; i < 300; i++) {
        Token t{};
        t.issued_at = std::chrono::system_clock::now().time_since_epoch().count();
        t.expires_at = t.issued_at + 30;
        sling.queue_token(t);
    }
    
    std::cout << "Queued 300 tokens — waiting for validation...\n";
    
    // Let it run for 2 seconds
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    uint64_t validated = sling.tokens_validated();
    uint64_t avg_lat   = sling.avg_latency_us();
    
    std::cout << "\n╔════════════════════════════════════╗\n";
    std::cout <<   "║   RESULTS                          ║\n";
    std::cout <<   "╚════════════════════════════════════╝\n";
    std::cout << "  Tokens validated: " << validated << " / 300\n";
    std::cout << "  Avg latency:      " << avg_lat << " μs\n";
    std::cout << "\n";
    
    if (avg_lat < 1.0 && validated >= 290) {
        std::cout << "✓ SLINGSHOT WORKING — THROTTLE DEFEATED\n\n";
        return 0;
    } else {
        std::cout << "✗ SLINGSHOT NEEDS TUNING\n\n";
        return 1;
    }
}
