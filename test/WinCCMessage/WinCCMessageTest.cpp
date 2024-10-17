#include "gtest/gtest.h"
#include "communication-utils/WinCCRequest.h"
#include "communication-utils/WinCCResponse.h"
#include <optional>

namespace
{

TEST(WinCCMessageTest, Request)
{
    WinCCRequest reqR("TESTR,READ\n");

    WinCCRequest::Command testR = {
        "TESTR",
        WinCCRequest::Operation::Read,
        std::nullopt
    };

    EXPECT_EQ(reqR.getCommands().size(), 1);

    EXPECT_EQ(reqR.getCommands()[0].name, testR.name);
    EXPECT_EQ(reqR.getCommands()[0].operation, testR.operation);
    EXPECT_EQ(reqR.getCommands()[0].value, testR.value);

    WinCCRequest reqW("TESTW,WRITE,-15.7\nTESTH,WRITE,0xbeef");

    WinCCRequest::Command testW = {
        "TESTW",
        WinCCRequest::Operation::Write,
        -15.7
    };

    WinCCRequest::Command testH = {
        "TESTH",
        WinCCRequest::Operation::Write,
        48879
    };

    EXPECT_EQ(reqW.getCommands().size(), 2);

    EXPECT_EQ(reqW.getCommands()[0].name, testW.name);
    EXPECT_EQ(reqW.getCommands()[0].operation, testW.operation);
    EXPECT_EQ(reqW.getCommands()[0].value, testW.value);

    EXPECT_EQ(reqW.getCommands()[1].name, testH.name);
    EXPECT_EQ(reqW.getCommands()[1].operation, testH.operation);
    EXPECT_EQ(reqW.getCommands()[1].value, testH.value);
}

TEST(WinCCMessageTest, MixingRejection)
{
    string resultRW = "OK";
    try {
        WinCCRequest reqRW("TESTR,READ\nTESTW,WRITE,0");
    } catch (const runtime_error& e) {
        resultRW = e.what();
    }

    EXPECT_EQ(resultRW, "TESTW: attempted operation mixing in single request");

    string resultWR = "OK";
    try {
        WinCCRequest reqWR("TESTR,WRITE,0\nTESTR,READ\n");
    } catch (const runtime_error& e) {
        resultWR = e.what();
    }

    EXPECT_EQ(resultWR, "TESTR: attempted operation mixing in single request");
}

TEST(WinCCMessageTest, Response)
{
    WinCCResponse res;
    res.addParameter("TEST1", { 0.7 }).addParameter("TEST2", { -123, 27.8, 312.9 }).addParameter("TEST3", {});

    EXPECT_EQ(res.getContents(), "TEST1,0.7\nTEST2,-123,27.8,312.9\nTEST3\n");
}

} // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}