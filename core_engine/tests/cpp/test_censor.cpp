#include "fc_flow/enforcement/censor.hpp"
#include <gtest/gtest.h>

using namespace mesh;

class CensorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use nullptr for dependencies not yet implemented
        censor_ = new Censor(1, nullptr, nullptr, nullptr, nullptr);
        censor_->init(1000);
    }

    void TearDown() override {
        censor_->shutdown();
        delete censor_;
    }

    Censor* censor_;
};

TEST_F(CensorTest, Initialization) {
    EXPECT_EQ(censor_->get_sector_id(), 1);
    EXPECT_EQ(censor_->get_case_record_count(), 0);
    EXPECT_EQ(censor_->get_sit_op_count(), 0);
}

TEST_F(CensorTest, EnforceRequiresWarrant) {
    // Without ERM, enforce should return NO_WARRANT
    std::string target_id = "1001";
    CensorResult result = censor_->enforce(target_id, CensorMode::ADVISE, "Test advisory", 1100);
    
    EXPECT_EQ(result, CensorResult::NO_WARRANT);
    EXPECT_EQ(censor_->get_case_record_count(), 0) << "No case should be created without warrant";
}

TEST_F(CensorTest, ZeroToleranceNoWarrantNeeded) {
    // Zero tolerance does NOT require warrant
    std::string target_id = "2001";
    CensorResult result = censor_->enforce_zero_tolerance(
        target_id, 
        IncidentType::HOSTILE_CONNECT, 
        "Hostile connection detected", 
        1100
    );
    
    EXPECT_EQ(result, CensorResult::CLEAR);
    EXPECT_EQ(censor_->get_enforcement_count(target_id), 1);
    EXPECT_EQ(censor_->get_case_record_count(), 1) << "Case should be created for zero tolerance";
}

TEST_F(CensorTest, ZeroToleranceValidIncidentsOnly) {
    // Only specific incidents qualify for zero tolerance
    std::string target_id = "3001";
    
    // Valid zero tolerance incident
    CensorResult result1 = censor_->enforce_zero_tolerance(
        target_id, 
        IncidentType::ROOM_FALL, 
        "Room fall detected", 
        1100
    );
    EXPECT_EQ(result1, CensorResult::CLEAR);
    
    // Invalid zero tolerance incident (should be DENIED)
    std::string target_id2 = "3002";
    CensorResult result2 = censor_->enforce_zero_tolerance(
        target_id2, 
        IncidentType::OTHER, 
        "Other incident", 
        1200
    );
    EXPECT_EQ(result2, CensorResult::DENIED);
}

TEST_F(CensorTest, PurgeRequiresSectorZero) {
    // Sector 1 cannot purge
    std::string target_id = "4001";
    CensorResult result = censor_->purge(target_id, "Should fail", 1100);
    
    EXPECT_EQ(result, CensorResult::DENIED);
    
    // Sector Zero can purge (but still needs warrant without ERM)
    Censor censor_zero(0, nullptr, nullptr, nullptr, nullptr);
    censor_zero.init(1000);
    
    CensorResult result2 = censor_zero.purge(target_id, "Sector Zero attempt", 1100);
    EXPECT_EQ(result2, CensorResult::NO_WARRANT) << "Still needs warrant even in Sector Zero";
    
    censor_zero.shutdown();
}

TEST_F(CensorTest, CaseRecordTracking) {
    std::string target_id = "5001";
    
    // Create case via zero tolerance
    censor_->enforce_zero_tolerance(target_id, IncidentType::MESH_PROBE, "Probe detected", 1100);
    
    // Verify case exists
    const CensorRecord* rec = censor_->get_case(target_id);
    ASSERT_NE(rec, nullptr);
    EXPECT_EQ(rec->target_id, target_id);
    EXPECT_EQ(rec->enforcement_count, 1);
    EXPECT_TRUE(rec->zero_tolerance);
    EXPECT_TRUE(rec->executed);
    EXPECT_FALSE(rec->escalated);
}

TEST_F(CensorTest, EscalationThreshold) {
    std::string target_id = "6001";
    
    // Hit target 3 times (escalation threshold)
    for (int i = 0; i < 3; i++) {
        censor_->enforce_zero_tolerance(
            target_id, 
            IncidentType::HOSTILE_CONNECT, 
            "Repeated violation", 
            1100 + i
        );
    }
    
    // After 3rd hit, should be escalated
    EXPECT_TRUE(censor_->is_escalated(target_id));
    EXPECT_EQ(censor_->get_sit_op_count(), 1) << "SIT operation should be opened";
}

TEST_F(CensorTest, MultipleTargets) {
    // Create cases for multiple targets
    censor_->enforce_zero_tolerance("7001", IncidentType::TOKEN_FORGE, "Forge attempt", 1100);
    censor_->enforce_zero_tolerance("7002", IncidentType::FUEL_DRAIN, "Fuel drain", 1200);
    censor_->enforce_zero_tolerance("7003", IncidentType::REPLAY_ATTACK, "Replay attack", 1300);
    
    EXPECT_EQ(censor_->get_case_record_count(), 3);
    EXPECT_EQ(censor_->get_enforcement_count("7001"), 1);
    EXPECT_EQ(censor_->get_enforcement_count("7002"), 1);
    EXPECT_EQ(censor_->get_enforcement_count("7003"), 1);
}

TEST_F(CensorTest, PrintStatus) {
    // Should not crash
    censor_->print_status();
    SUCCEED();
}

TEST_F(CensorTest, PrintCaseLog) {
    // Create a case first
    censor_->enforce_zero_tolerance("8001", IncidentType::ACCOUNT_TAKEOVER, "Takeover detected", 1100);
    
    // Should not crash
    censor_->print_case_log();
    SUCCEED();
}

