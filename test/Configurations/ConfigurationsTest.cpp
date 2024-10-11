#include "gtest/gtest.h"

#include "Configurations.h"
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
    { "DELAY_C", Board::ParameterInfo("DELAY_C", 0x1, 0, 16, 1, Board::ParameterInfo::ValueEncoding::Signed, -1e16, 1e16, Equation::Empty(), Equation::Empty(), false, false) }
};

// Fails due to value storage logic change (using board) - worked before though, should work now
// TEST(ConfigurationsTest, Delays)
// {
//     shared_ptr<Board> tcm = make_shared<Board>("TCM", 0, nullptr, nullptr);
//     tcm->emplace(testMap.at("DELAY_A"));
//     tcm->emplace(testMap.at("DELAY_C"));

//     Configurations::TcmConfigurations tcmCfg(tcm);

//     auto noChange = tcmCfg.processDelayInput(nullopt, nullopt);
//     EXPECT_EQ(tcmCfg.getDelayA(), nullopt);
//     EXPECT_EQ(tcmCfg.getDelayC(), nullopt);
//     EXPECT_EQ(tcmCfg.m_delayDifference, 0);
//     EXPECT_EQ(noChange, nullopt);

//     auto aChange = tcmCfg.processDelayInput(8, nullopt);
//     auto aChangeExpected = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x0, { 0xffff0000, 0x8 }),
//                                          SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x0, {}, true) });
//     EXPECT_EQ(tcmCfg.getDelayA(), 8);
//     EXPECT_EQ(tcmCfg.getDelayC(), nullopt);
//     EXPECT_EQ(tcmCfg.m_delayDifference, 8);
//     EXPECT_EQ(aChange->getSequence(), aChangeExpected.getSequence());

//     auto cChange = tcmCfg.processDelayInput(nullopt, 4);
//     auto cChangeExpected = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x1, { 0xffff0000, 0x4 }),
//                                          SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true) });
//     EXPECT_EQ(tcmCfg.getDelayA(), 8);
//     EXPECT_EQ(tcmCfg.getDelayC(), 4);
//     EXPECT_EQ(tcmCfg.m_delayDifference, 4);
//     EXPECT_EQ(cChange->getSequence(), cChangeExpected.getSequence());

//     auto aLargerChange = tcmCfg.processDelayInput(0, 8);
//     auto aLargerChangeExpected = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x1, { 0xffff0000, 0x8 }),
//                                                SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x0, { 0xffff0000, 0x0 }),
//                                                SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true),
//                                                SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x0, {}, true) });
//     EXPECT_EQ(tcmCfg.getDelayA(), 0);
//     EXPECT_EQ(tcmCfg.getDelayC(), 8);
//     EXPECT_EQ(tcmCfg.m_delayDifference, 8);
//     EXPECT_EQ(aLargerChange->getSequence(), aLargerChangeExpected.getSequence());

//     auto cLargerChange = tcmCfg.processDelayInput(0, -1);
//     auto cLargerChangeExpected = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x1, { 0xffff0000, 0xffff }),
//                                                SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x0, { 0xffff0000, 0x0 }),
//                                                SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true),
//                                                SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x0, {}, true) });
//     EXPECT_EQ(tcmCfg.getDelayA(), 0);
//     EXPECT_EQ(tcmCfg.getDelayC(), -1);
//     EXPECT_EQ(tcmCfg.m_delayDifference, 9);
//     EXPECT_EQ(cLargerChange->getSequence(), cLargerChangeExpected.getSequence());
// }

TEST(ConfigurationsTest, PmPim)
{
    DatabaseInterface::s_queryResults["SELECT parameter_name, parameter_value FROM parameters p JOIN configurations c ON p.parameter_id = c.parameter_id WHERE configuration_name = 'TEST' AND board_name = 'PMA7';"] = {
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

    DatabaseInterface::s_queryResults["SELECT parameter_name, parameter_value FROM parameters p JOIN configurations c ON p.parameter_id = c.parameter_id WHERE configuration_name = 'TEST' AND board_name = 'TCM';"] = {
        { new Box<string>("UNSIGNED_HALF"), new Box<double>(7.) },
        { new Box<string>("SIGNED_HALF"), new Box<double>(0.) },
    };

    Configurations::TcmConfigurations tcmCfg(tcm);

    string tcmPimIdleNoDelaysResult = tcmCfg.processInputMessage("TEST");
    auto tcmPimIdleNoDelaysExpected = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0xf00d, { 0x0, 0x00070000 }),
                                                    SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0xf00d, {}, true) });
    EXPECT_EQ(tcmPimIdleNoDelaysResult, tcmPimIdleNoDelaysExpected.getSequence());
    EXPECT_EQ(tcmCfg.m_state, Configurations::TcmConfigurations::State::ApplyingData);
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

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace