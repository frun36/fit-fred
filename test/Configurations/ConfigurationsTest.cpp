#include "gtest/gtest.h"

#include "services/Configurations.h"
#include "Board.h"
#include "Equation.h"

namespace
{

unordered_map<string, Board::ParameterInfo> testMap = {
    { "SIGNED_REG", Board::ParameterInfo("SIGNED_REG", 0xbeef, 0, 32, 1, Board::ParameterInfo::ValueEncoding::Signed, 0, 100, Equation::Empty(), Equation::Empty(), false, false) },
    { "SIGNED_HALF", Board::ParameterInfo("SIGNED_HALF", 0xf00d, 0, 16, 1, Board::ParameterInfo::ValueEncoding::Signed, -100, 100, Equation::Empty(), Equation::Empty(), false, false) },
    { "UNSIGNED_HALF", Board::ParameterInfo("UNSIGNED_HALF", 0xf00d, 16, 16, 1, Board::ParameterInfo::ValueEncoding::Unsigned, 0, 100, Equation::Empty(), Equation::Empty(), false, false) },
    { "READONLY_FLAG", Board::ParameterInfo("READONLY_FLAG", 0xbeef, 7, 1, 1, Board::ParameterInfo::ValueEncoding::Unsigned, 0, 1, Equation::Empty(), Equation::Empty(), false, true) },
    { "DELAY_A", Board::ParameterInfo("DELAY_A", 0x0, 0, 16, 1, Board::ParameterInfo::ValueEncoding::Signed, -1e16, 1e16, Equation::Empty(), Equation::Empty(), false, false) },
    { "DELAY_C", Board::ParameterInfo("DELAY_C", 0x1, 0, 16, 1, Board::ParameterInfo::ValueEncoding::Signed, -1e16, 1e16, Equation::Empty(), Equation::Empty(), false, false) },
    { "BOARD_STATUS_SYSTEM_RESET", Board::ParameterInfo("BOARD_STATUS_SYSTEM_RESET", 0x2, 0, 1, 1, Board::ParameterInfo::ValueEncoding::Unsigned, 0, 1, Equation::Empty(), Equation::Empty(), false, false) }
};

TEST(ConfigurationsTest, PmPim)
{
    DatabaseInterface::s_queryResults["SELECT parameter_name, parameter_value FROM configuration_parameters WHERE configuration_name = 'TEST' AND board_name = 'PMA7'"] = {
        { new Box<string>("UNSIGNED_HALF"), new Box<double>(7.) },
        { new Box<string>("SIGNED_HALF"), new Box<double>(0.) },
    };

    shared_ptr<Board> pm = make_shared<Board>("PMA7", 0, nullptr, nullptr);
    pm->emplace(testMap.at("UNSIGNED_HALF"));
    pm->emplace(testMap.at("SIGNED_HALF"));
    Configurations::PmConfigurations pmCfg(pm);
    string pmPimResult = pmCfg.processInputMessage("TEST");
    auto pmPimExpected = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0xf00d, { 0x0, 0x00070000 }),
                                       SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0xf00d, {}, true) });
    EXPECT_EQ(pmPimResult, pmPimExpected.getSequence());
}

TEST(ConfigurationsTest, Tcm)
{
    shared_ptr<Board> tcm = make_shared<Board>("TCM", 0, nullptr, nullptr);
    tcm->emplace(testMap.at("DELAY_A"));
    tcm->emplace(testMap.at("DELAY_C"));
    tcm->emplace(testMap.at("UNSIGNED_HALF"));
    tcm->emplace(testMap.at("SIGNED_HALF"));
    tcm->emplace(testMap.at("BOARD_STATUS_SYSTEM_RESET"));

    Configurations::TcmConfigurations tcmCfg(tcm);

    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::Idle);
    EXPECT_EQ(tcmCfg.m_configurationName, nullopt);
    EXPECT_EQ(tcmCfg.m_configurationInfo, nullopt);
    EXPECT_EQ(tcmCfg.m_delayDifference, 0);
    EXPECT_EQ(tcmCfg.m_delayResponse, nullopt);

    // Alf response - idle
    EXPECT_THROW(tcmCfg.processOutputMessage("success"), runtime_error);

    // Send test - idle, no delay
    DatabaseInterface::s_queryResults["SELECT parameter_name, parameter_value FROM configuration_parameters WHERE configuration_name = 'TEST' AND board_name = 'TCM'"] = {
        { new Box<string>("UNSIGNED_HALF"), new Box<double>(7.) },
        { new Box<string>("SIGNED_HALF"), new Box<double>(0.) },
    };

    string tcmPimIdleNoDelaysResult = tcmCfg.processInputMessage("TEST");
    auto tcmPimIdleNoDelaysExpected = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0xf00d, { 0x0, 0x00070000 }),
                                                    SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0xf00d, {}, true) });
    EXPECT_EQ(tcmPimIdleNoDelaysResult, tcmPimIdleNoDelaysExpected.getSequence());
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::ApplyingData);
    EXPECT_EQ(tcmCfg.m_configurationName, "TEST");
    EXPECT_EQ(tcmCfg.m_configurationInfo->req, "UNSIGNED_HALF,WRITE,7.000000\nSIGNED_HALF,WRITE,0.000000\n");
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayA, nullopt);
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayC, nullopt);
    EXPECT_EQ(tcmCfg.m_delayDifference, 0);
    EXPECT_EQ(tcmCfg.m_delayResponse, nullopt);

    // Send test - busy
    EXPECT_THROW(tcmCfg.processInputMessage("TEST"), runtime_error);

    // Alf response - successful
    string tcmPomApplyingDataNoDelaysResult = tcmCfg.processOutputMessage("success\n0\n0\n0x0000000F00D00070000\n");
    string tcmPomApplyingDataNoDelaysExpected = "UNSIGNED_HALF,7\nSIGNED_HALF,0\n";
    EXPECT_EQ(tcmPomApplyingDataNoDelaysResult, tcmPomApplyingDataNoDelaysExpected);
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::Idle);
    EXPECT_EQ(tcmCfg.m_configurationName, nullopt);
    EXPECT_EQ(tcmCfg.m_configurationInfo, nullopt);
    EXPECT_EQ(tcmCfg.m_delayDifference, 0);
    EXPECT_EQ(tcmCfg.m_delayResponse, nullopt);

    // Send test - _CONTINUE while idle
    EXPECT_THROW(tcmCfg.processInputMessage("_CONTINUE"), runtime_error);

    // Send test - idle, with delay
    DatabaseInterface::s_queryResults["SELECT parameter_name, parameter_value FROM configuration_parameters WHERE configuration_name = 'TEST' AND board_name = 'TCM'"] = {
        { new Box<string>("UNSIGNED_HALF"), new Box<double>(7.) },
        { new Box<string>("SIGNED_HALF"), new Box<double>(0.) },
        { new Box<string>("DELAY_A"), new Box<double>(5.) },
        { new Box<string>("DELAY_C"), new Box<double>(-1.) }
    };

    string tcmPimIdleDelaysResult = tcmCfg.processInputMessage("TEST");
    auto tcmPimIdleDelaysExpected = SwtSequence({
        SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x1, { 0xffff0000, 0xffff }),
        SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x0, { 0xffff0000, 0x5 }),
        SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true),
        SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x0, {}, true),
    });
    EXPECT_EQ(tcmPimIdleDelaysResult, tcmPimIdleDelaysExpected.getSequence());
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::ApplyingDelays);
    EXPECT_EQ(tcmCfg.m_configurationName, "TEST");
    EXPECT_EQ(tcmCfg.m_configurationInfo->req, "UNSIGNED_HALF,WRITE,7.000000\nSIGNED_HALF,WRITE,0.000000\nBOARD_STATUS_SYSTEM_RESET,WRITE,1\n");
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayA, 5);
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayC, -1);
    EXPECT_EQ(tcmCfg.m_delayDifference, 5);
    EXPECT_EQ(tcmCfg.m_delayResponse, nullopt);

    // Alf - successful delay update
    string tcmPomApplyingDelaysResult = tcmCfg.processOutputMessage("success\n0\n0\n0x000000000010000ffff\n0x0000000000000000005\n");
    string tcmPomApplyingDelaysExpected = "";
    EXPECT_EQ(tcmPomApplyingDelaysResult, tcmPomApplyingDelaysExpected);
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::DelaysApplied);
    EXPECT_EQ(tcmCfg.m_configurationName, "TEST");
    EXPECT_EQ(tcmCfg.m_configurationInfo->req, "UNSIGNED_HALF,WRITE,7.000000\nSIGNED_HALF,WRITE,0.000000\nBOARD_STATUS_SYSTEM_RESET,WRITE,1\n");
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayA, 5);
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayC, -1);
    EXPECT_EQ(tcmCfg.m_delayDifference, 5);
    EXPECT_EQ(tcmCfg.m_delayResponse, "DELAY_C,-1\nDELAY_A,5\n");

    // Idle - delays applied, not _CONTINUE
    EXPECT_THROW(tcmCfg.processInputMessage("TEST"), runtime_error);

    // Alf response - delays applied
    EXPECT_THROW(tcmCfg.processOutputMessage("success"), runtime_error);

    // Idle - _CONTINUE
    string tcmPimIdleDelaysContinueResult = tcmCfg.processInputMessage("_CONTINUE");
    auto tcmPimIdleDelaysContinueExpected = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x2, { 0xfffffffe, 0x1 }),
                                                          SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0xf00d, { 0x0, 0x00070000 }),
                                                          SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x2, {}, true),
                                                          SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0xf00d, {}, true) });
    EXPECT_EQ(tcmPimIdleDelaysContinueResult, tcmPimIdleDelaysContinueExpected.getSequence());
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::ApplyingData);
    EXPECT_EQ(tcmCfg.m_configurationName, "TEST");
    EXPECT_EQ(tcmCfg.m_configurationInfo->req, "UNSIGNED_HALF,WRITE,7.000000\nSIGNED_HALF,WRITE,0.000000\nBOARD_STATUS_SYSTEM_RESET,WRITE,1\n");
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayA, 5);
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayC, -1);
    EXPECT_EQ(tcmCfg.m_delayDifference, 5);
    EXPECT_EQ(tcmCfg.m_delayResponse, "DELAY_C,-1\nDELAY_A,5\n");

    // Alf - successful completion
    string tcmPomApplyingDataDelaysResult = tcmCfg.processOutputMessage("success\n0\n0\n0x0000000F00D00070000\n");
    string tcmPomApplyingDataDelaysExpected = "DELAY_C,-1\nDELAY_A,5\nUNSIGNED_HALF,7\nSIGNED_HALF,0\n";
    EXPECT_EQ(tcmPomApplyingDataDelaysResult, tcmPomApplyingDataDelaysExpected);
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::Idle);
    EXPECT_EQ(tcmCfg.m_configurationName, nullopt);
    EXPECT_EQ(tcmCfg.m_configurationInfo, nullopt);
    EXPECT_EQ(tcmCfg.m_delayDifference, 0);
    EXPECT_EQ(tcmCfg.m_delayResponse, nullopt);

    // No delay - ALF failure test
    DatabaseInterface::s_queryResults["SELECT parameter_name, parameter_value FROM configuration_parameters WHERE configuration_name = 'TEST' AND board_name = 'TCM'"] = {
        { new Box<string>("UNSIGNED_HALF"), new Box<double>(7.) },
        { new Box<string>("SIGNED_HALF"), new Box<double>(0.) },
    };

    string tcmPimIdleNoDelaysResult2 = tcmCfg.processInputMessage("TEST");
    auto tcmPimIdleNoDelaysExpected2 = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0xf00d, { 0x0, 0x00070000 }),
                                                     SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0xf00d, {}, true) });
    EXPECT_EQ(tcmPimIdleNoDelaysResult2, tcmPimIdleNoDelaysExpected2.getSequence());
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::ApplyingData);
    EXPECT_EQ(tcmCfg.m_configurationName, "TEST");
    EXPECT_EQ(tcmCfg.m_configurationInfo->req, "UNSIGNED_HALF,WRITE,7.000000\nSIGNED_HALF,WRITE,0.000000\n");
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayA, nullopt);
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayC, nullopt);
    EXPECT_EQ(tcmCfg.m_delayDifference, 0);
    EXPECT_EQ(tcmCfg.m_delayResponse, nullopt);

    string tcmPimNoDelaysErrorResponse = tcmCfg.processOutputMessage("failure\n");
    EXPECT_EQ(tcmPimNoDelaysErrorResponse, "TCM configuration TEST was not applied\nERROR - SEQUENCE: ALF COMMUNICATION FAILED\n");
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::Idle);
    EXPECT_EQ(tcmCfg.m_configurationName, nullopt);
    EXPECT_EQ(tcmCfg.m_configurationInfo, nullopt);
    EXPECT_EQ(tcmCfg.m_delayDifference, 0);
    EXPECT_EQ(tcmCfg.m_delayResponse, nullopt);

    // Delay - ALF failure on Delay
    DatabaseInterface::s_queryResults["SELECT parameter_name, parameter_value FROM configuration_parameters WHERE configuration_name = 'TEST' AND board_name = 'TCM'"] = {
        { new Box<string>("UNSIGNED_HALF"), new Box<double>(7.) },
        { new Box<string>("SIGNED_HALF"), new Box<double>(0.) },
        { new Box<string>("DELAY_A"), new Box<double>(0.) },
        { new Box<string>("DELAY_C"), new Box<double>(0.) }
    };

    string tcmPimIdleDelaysResult2 = tcmCfg.processInputMessage("TEST");
    auto tcmPimIdleDelaysExpected2 = SwtSequence({
        SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x1, { 0xffff0000, 0x0 }),
        SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x0, { 0xffff0000, 0x0 }),
        SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true),
        SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x0, {}, true),
    });
    EXPECT_EQ(tcmPimIdleDelaysResult2, tcmPimIdleDelaysExpected2.getSequence());
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::ApplyingDelays);
    EXPECT_EQ(tcmCfg.m_configurationName, "TEST");
    EXPECT_EQ(tcmCfg.m_configurationInfo->req, "UNSIGNED_HALF,WRITE,7.000000\nSIGNED_HALF,WRITE,0.000000\nBOARD_STATUS_SYSTEM_RESET,WRITE,1\n");
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayA, 0);
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayC, 0);
    EXPECT_EQ(tcmCfg.m_delayDifference, 5);
    EXPECT_EQ(tcmCfg.m_delayResponse, nullopt);

    string tcmPimDelaysDelayErrorResponse = tcmCfg.processOutputMessage("failure\n");
    EXPECT_EQ(tcmPimDelaysDelayErrorResponse, "TCM configuration TEST was not applied: delay change failed\nERROR - SEQUENCE: ALF COMMUNICATION FAILED\n");
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::Idle);
    EXPECT_EQ(tcmCfg.m_configurationName, nullopt);
    EXPECT_EQ(tcmCfg.m_configurationInfo, nullopt);
    EXPECT_EQ(tcmCfg.m_delayDifference, 0);
    EXPECT_EQ(tcmCfg.m_delayResponse, nullopt);

    // Delay - ALF failure on Data
    DatabaseInterface::s_queryResults["SELECT parameter_name, parameter_value FROM configuration_parameters WHERE configuration_name = 'TEST' AND board_name = 'TCM'"] = {
        { new Box<string>("UNSIGNED_HALF"), new Box<double>(7.) },
        { new Box<string>("SIGNED_HALF"), new Box<double>(0.) },
        { new Box<string>("DELAY_A"), new Box<double>(0.) },
        { new Box<string>("DELAY_C"), new Box<double>(0.) }
    };

    string tcmPimIdleDelaysResult3 = tcmCfg.processInputMessage("TEST");
    auto tcmPimIdleDelaysExpected3 = SwtSequence({
        SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x1, { 0xffff0000, 0x0 }),
        SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x0, { 0xffff0000, 0x0 }),
        SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true),
        SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x0, {}, true),
    });
    EXPECT_EQ(tcmPimIdleDelaysResult3, tcmPimIdleDelaysExpected3.getSequence());
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::ApplyingDelays);
    EXPECT_EQ(tcmCfg.m_configurationName, "TEST");
    EXPECT_EQ(tcmCfg.m_configurationInfo->req, "UNSIGNED_HALF,WRITE,7.000000\nSIGNED_HALF,WRITE,0.000000\nBOARD_STATUS_SYSTEM_RESET,WRITE,1\n");
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayA, 0);
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayC, 0);
    EXPECT_EQ(tcmCfg.m_delayDifference, 5);
    EXPECT_EQ(tcmCfg.m_delayResponse, nullopt);

    string tcmPomApplyingDelaysResult3 = tcmCfg.processOutputMessage("success\n0\n0\n0x0000000000100000000\n0x0000000000000000000\n");
    string tcmPomApplyingDelaysExpected3 = "";
    EXPECT_EQ(tcmPomApplyingDelaysResult, tcmPomApplyingDelaysExpected);
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::DelaysApplied);
    EXPECT_EQ(tcmCfg.m_configurationName, "TEST");
    EXPECT_EQ(tcmCfg.m_configurationInfo->req, "UNSIGNED_HALF,WRITE,7.000000\nSIGNED_HALF,WRITE,0.000000\nBOARD_STATUS_SYSTEM_RESET,WRITE,1\n");
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayA, 0);
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayC, 0);
    EXPECT_EQ(tcmCfg.m_delayDifference, 5);
    EXPECT_EQ(tcmCfg.m_delayResponse, "DELAY_C,0\nDELAY_A,0\n");

    EXPECT_THROW(tcmCfg.processInputMessage("TEST"), runtime_error);
    EXPECT_THROW(tcmCfg.processOutputMessage("success"), runtime_error);

    string tcmPimIdleDelaysContinueResult3 = tcmCfg.processInputMessage("_CONTINUE");
    EXPECT_EQ(tcmPimIdleDelaysContinueResult3, tcmPimIdleDelaysContinueExpected.getSequence());
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::ApplyingData);
    EXPECT_EQ(tcmCfg.m_configurationName, "TEST");
    EXPECT_EQ(tcmCfg.m_configurationInfo->req, "UNSIGNED_HALF,WRITE,7.000000\nSIGNED_HALF,WRITE,0.000000\nBOARD_STATUS_SYSTEM_RESET,WRITE,1\n");
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayA, 0);
    EXPECT_EQ(tcmCfg.m_configurationInfo->delayC, 0);
    EXPECT_EQ(tcmCfg.m_delayDifference, 5);
    EXPECT_EQ(tcmCfg.m_delayResponse, "DELAY_C,0\nDELAY_A,0\n");

    string tcmPomDelaysDataErrorResponse = tcmCfg.processOutputMessage("failure");
    EXPECT_EQ(tcmPomDelaysDataErrorResponse, "TCM configuration TEST was applied partially\nDELAY_C,0\nDELAY_A,0\nERROR - SEQUENCE: ALF COMMUNICATION FAILED\n");
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::Idle);
    EXPECT_EQ(tcmCfg.m_configurationName, nullopt);
    EXPECT_EQ(tcmCfg.m_configurationInfo, nullopt);
    EXPECT_EQ(tcmCfg.m_delayDifference, 0);
    EXPECT_EQ(tcmCfg.m_delayResponse, nullopt);

    /*
    States - Idle, ApplyingDelays, DelaysApplied, ApplyingData
    PIM:
     - Idle - [NAME] (no delays)
     - Idle - [NAME] (delays)
     - Idle - _CONTINUE
     - ApplyingDelays - *
     - DelaysApplied - [NAME]
     - DelaysApplied - _CONTINUE
     - ApplyingData - *
    POM
     - Idle - *
     - ApplyingDelays - *
     - DelaysApplied - *
     - ApplyingData - *
    */
}

} // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}