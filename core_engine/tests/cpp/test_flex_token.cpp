#include "fc_flow/coin/flex_token.hpp"
#include <gtest/gtest.h>
#include <sodium.h>
#include <thread>

using namespace fc_flow::coin;

class FlexTokenTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_GE(sodium_init(), 0) << "libsodium initialization failed";
    }

    FlexInput make_test_input() {
        FlexInput input;
        randombytes_buf(input.challenge_hash.data(), 32);
        input.flow_cycle = 12345;
        input.sector_id = 2;
        input.stack_id = 0x01;
        return input;
    }
};

TEST_F(FlexTokenTest, MintFlex_Success) {
    auto input = make_test_input();
    auto result = mint_flex(input);
    
    ASSERT_TRUE(result.has_value()) << "mint_flex failed: "
                                     << to_string(result.error());
    
    const FlexToken& token = result.value();
    EXPECT_EQ(token.sector_id, 2);
    EXPECT_EQ(token.stack_id, 0x01);
    EXPECT_GT(token.timestamp_ns, 0);
    
    bool pin_b_nonzero = false;
    for (auto byte : token.pin_b) {
        if (byte != 0) {
            pin_b_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(pin_b_nonzero) << "PIN B should not be all zeros";
}

TEST_F(FlexTokenTest, VerifyFlex_Success) {
    auto input = make_test_input();
    auto mint_result = mint_flex(input);
    ASSERT_TRUE(mint_result.has_value()) << "mint_flex failed: "
                                          << to_string(mint_result.error());
    
    auto verify_result = verify_flex(mint_result.value(), input);
    EXPECT_TRUE(verify_result.has_value()) << "verify_flex failed: "
                                            << to_string(verify_result.error());
}

TEST_F(FlexTokenTest, VerifyFlex_StaleToken) {
    auto input = make_test_input();
    auto mint_result = mint_flex(input);
    ASSERT_TRUE(mint_result.has_value()) << "mint_flex failed: "
                                          << to_string(mint_result.error());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    
    auto verify_result = verify_flex(mint_result.value(), input);
    EXPECT_FALSE(verify_result.has_value()) << "Expected verification to fail for stale token";
    if (!verify_result.has_value()) {
        EXPECT_EQ(verify_result.error(), FlexError::STALE_TOKEN);
    }
}

TEST_F(FlexTokenTest, MintFlex_InvalidSector) {
    auto input = make_test_input();
    input.sector_id = 7;  // Invalid (must be 0-6)
    
    auto result = mint_flex(input);
    EXPECT_FALSE(result.has_value()) << "Expected minting to fail for invalid sector";
    if (!result.has_value()) {
        EXPECT_EQ(result.error(), FlexError::INVALID_SECTOR);
    }
}

TEST_F(FlexTokenTest, MintFlex_InvalidStack) {
    auto input = make_test_input();
    input.stack_id = 0x03;  // Invalid (must be 0x01 or 0x02)
    
    auto result = mint_flex(input);
    EXPECT_FALSE(result.has_value()) << "Expected minting to fail for invalid stack";
    if (!result.has_value()) {
        EXPECT_EQ(result.error(), FlexError::INVALID_STACK);
    }
}

TEST_F(FlexTokenTest, FlexTokenWipe) {
    auto input = make_test_input();
    auto result = mint_flex(input);
    ASSERT_TRUE(result.has_value()) << "mint_flex failed: "
                                     << to_string(result.error());
    
    FlexToken token = result.value();
    token.wipe();
    
    for (auto byte : token.pin_b) EXPECT_EQ(byte, 0);
    EXPECT_EQ(token.timestamp_ns, 0);
}
