#include "fc_flow/crypto/crossbolt.hpp"
#include "fc_flow/enforcement/monitus.hpp"
#include <gtest/gtest.h>
#include <sodium.h>
#include <thread>

using namespace fc_flow::crypto;
using namespace fc_flow::enforcement;

class CrossBoltTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_GE(sodium_init(), 0) << "libsodium initialization failed";
    }

    BoltInput make_test_input() {
        BoltInput input;
        randombytes_buf(input.master_secret.data(), 32);
        randombytes_buf(input.hardware_dna.data(), 64);
        randombytes_buf(input.entropy.data(), 32);
        input.flow_cycle = 12345;
        input.sector_id = 2;
        input.stack_id = 0x01;
        input.tick_salt = 67890;
        return input;
    }
};

TEST_F(CrossBoltTest, FireCrossBolt_Success) {
    auto input = make_test_input();
    auto result = fire_crossbolt(input);
    
    ASSERT_TRUE(result.has_value()) << "fire_crossbolt failed: " 
                                     << to_string(result.error());
    
    const CrossBolt& lock = result.value();
    EXPECT_EQ(lock.sector_id, 2);
    EXPECT_EQ(lock.stack_id, 0x01);
    EXPECT_GT(lock.timestamp_ns, 0);
    
    bool pin_a_nonzero = false;
    for (auto byte : lock.pin_a) {
        if (byte != 0) {
            pin_a_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(pin_a_nonzero) << "PIN A should not be all zeros";
}

TEST_F(CrossBoltTest, VerifyCrossBolt_Success) {
    auto input = make_test_input();
    auto gen_result = fire_crossbolt(input);
    ASSERT_TRUE(gen_result.has_value()) << "fire_crossbolt failed: "
                                         << to_string(gen_result.error());
    
    auto verify_result = verify_crossbolt(gen_result.value(), input);
    EXPECT_TRUE(verify_result.has_value()) << "verify_crossbolt failed: "
                                            << to_string(verify_result.error());
}

TEST_F(CrossBoltTest, VerifyCrossBolt_StaleLock) {
    auto input = make_test_input();
    auto gen_result = fire_crossbolt(input);
    ASSERT_TRUE(gen_result.has_value()) << "fire_crossbolt failed: "
                                         << to_string(gen_result.error());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    
    auto verify_result = verify_crossbolt(gen_result.value(), input);
    EXPECT_FALSE(verify_result.has_value()) << "Expected verification to fail for stale lock";
    if (!verify_result.has_value()) {
        EXPECT_EQ(verify_result.error(), CrossBoltError::STALE_LOCK);
    }
}

TEST_F(CrossBoltTest, VerifyCrossBolt_WrongStack) {
    auto input = make_test_input();
    auto gen_result = fire_crossbolt(input);
    ASSERT_TRUE(gen_result.has_value()) << "fire_crossbolt failed: "
                                         << to_string(gen_result.error());
    
    auto wrong_input = input;
    wrong_input.stack_id = 0x02;
    
    auto verify_result = verify_crossbolt(gen_result.value(), wrong_input);
    EXPECT_FALSE(verify_result.has_value()) << "Expected verification to fail for wrong stack";
    if (!verify_result.has_value()) {
        EXPECT_EQ(verify_result.error(), CrossBoltError::BOLT_DID_NOT_FIRE);
    }
}

TEST_F(CrossBoltTest, MonitusWitness_BoltFired) {
    Monitus monitus;
    auto input = make_test_input();
    auto result = fire_crossbolt(input);
    ASSERT_TRUE(result.has_value()) << "fire_crossbolt failed: "
                                     << to_string(result.error());
    
    monitus.witness_bolt_fired(2, 12345, result.value());
    
    EXPECT_EQ(monitus.get_event_count(MonitusEventType::BOLT_FIRED), 1);
}

TEST_F(CrossBoltTest, TamperLockWipe) {
    auto input = make_test_input();
    auto result = fire_crossbolt(input);
    ASSERT_TRUE(result.has_value()) << "fire_crossbolt failed: "
                                     << to_string(result.error());
    
    CrossBolt lock = result.value();
    lock.wipe();
    
    for (auto byte : lock.pin_a) EXPECT_EQ(byte, 0);
    for (auto byte : lock.pin_b) EXPECT_EQ(byte, 0);
    for (auto byte : lock.bolt) EXPECT_EQ(byte, 0);
    EXPECT_EQ(lock.timestamp_ns, 0);
}
