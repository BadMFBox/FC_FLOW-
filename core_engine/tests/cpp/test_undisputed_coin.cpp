#include "fc_flow/coin/undisputed_coin.hpp"
#include <gtest/gtest.h>
#include <sodium.h>
#include <thread>

using namespace fc_flow::coin;

class UndisputedCoinTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_GE(sodium_init(), 0) << "libsodium initialization failed";
    }

    CoinInput make_test_input() {
        CoinInput input;
        randombytes_buf(input.master_secret.data(), 32);
        randombytes_buf(input.hardware_dna.data(), 64);
        randombytes_buf(input.entropy.data(), 32);
        input.flow_cycle = 12345;
        input.sector_id = 2;
        input.stack_id = 0x01;
        return input;
    }
};

TEST_F(UndisputedCoinTest, MintUndisputed_Success) {
    auto input = make_test_input();
    auto result = mint_undisputed(input);
    
    ASSERT_TRUE(result.has_value()) << "mint_undisputed failed: "
                                     << to_string(result.error());
    
    const UndisputedCoin& coin = result.value();
    EXPECT_EQ(coin.sector_id, 2);
    EXPECT_EQ(coin.stack_id, 0x01);
    EXPECT_GT(coin.timestamp_ns, 0);
    
    bool pin_a_nonzero = false;
    for (auto byte : coin.pin_a) {
        if (byte != 0) {
            pin_a_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(pin_a_nonzero) << "PIN A should not be all zeros";
}

TEST_F(UndisputedCoinTest, VerifyUndisputed_Success) {
    auto input = make_test_input();
    auto mint_result = mint_undisputed(input);
    ASSERT_TRUE(mint_result.has_value()) << "mint_undisputed failed: "
                                          << to_string(mint_result.error());
    
    auto verify_result = verify_undisputed(mint_result.value(), input);
    EXPECT_TRUE(verify_result.has_value()) << "verify_undisputed failed: "
                                            << to_string(verify_result.error());
}

TEST_F(UndisputedCoinTest, VerifyUndisputed_StaleCoin) {
    auto input = make_test_input();
    auto mint_result = mint_undisputed(input);
    ASSERT_TRUE(mint_result.has_value()) << "mint_undisputed failed: "
                                          << to_string(mint_result.error());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    
    auto verify_result = verify_undisputed(mint_result.value(), input);
    EXPECT_FALSE(verify_result.has_value()) << "Expected verification to fail for stale coin";
    if (!verify_result.has_value()) {
        EXPECT_EQ(verify_result.error(), CoinError::STALE_COIN);
    }
}

TEST_F(UndisputedCoinTest, MintUndisputed_InvalidSector) {
    auto input = make_test_input();
    input.sector_id = 7;  // Invalid (must be 0-6)
    
    auto result = mint_undisputed(input);
    EXPECT_FALSE(result.has_value()) << "Expected minting to fail for invalid sector";
    if (!result.has_value()) {
        EXPECT_EQ(result.error(), CoinError::INVALID_SECTOR);
    }
}

TEST_F(UndisputedCoinTest, MintUndisputed_InvalidStack) {
    auto input = make_test_input();
    input.stack_id = 0x03;  // Invalid (must be 0x01 or 0x02)
    
    auto result = mint_undisputed(input);
    EXPECT_FALSE(result.has_value()) << "Expected minting to fail for invalid stack";
    if (!result.has_value()) {
        EXPECT_EQ(result.error(), CoinError::INVALID_STACK);
    }
}

TEST_F(UndisputedCoinTest, CoinWipe) {
    auto input = make_test_input();
    auto result = mint_undisputed(input);
    ASSERT_TRUE(result.has_value()) << "mint_undisputed failed: "
                                     << to_string(result.error());
    
    UndisputedCoin coin = result.value();
    coin.wipe();
    
    for (auto byte : coin.pin_a) EXPECT_EQ(byte, 0);
    EXPECT_EQ(coin.timestamp_ns, 0);
}
