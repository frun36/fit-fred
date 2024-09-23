#include "gtest/gtest.h"
#include "WinCCRequest.h"
#include "WinCCResponse.h"
#include <optional>

namespace {

TEST(WinCCMessageTest, Request) {
    WinCCRequest reqR("TESTR,READ\nTESTW,WRITE,-15.7\nTESTH,WRITE,0xbeef");

    WinCCRequest::Command testR = { 
        "TESTR", 
        WinCCRequest::Command::Operation::Read, 
        std::nullopt 
    };

    WinCCRequest::Command testW = { 
        "TESTW", 
        WinCCRequest::Command::Operation::Write, 
        -15.7 
    };

    WinCCRequest::Command testH = { 
        "TESTH", 
        WinCCRequest::Command::Operation::Write, 
        48879 
    };

    EXPECT_EQ(reqR.getCommands().size(), 3);

    EXPECT_EQ(reqR.getCommands()[0].name, testR.name);
    EXPECT_EQ(reqR.getCommands()[0].operation, testR.operation);
    EXPECT_EQ(reqR.getCommands()[0].value, testR.value);

    EXPECT_EQ(reqR.getCommands()[1].name, testW.name);
    EXPECT_EQ(reqR.getCommands()[1].operation, testW.operation);
    EXPECT_EQ(reqR.getCommands()[1].value, testW.value);
    
    EXPECT_EQ(reqR.getCommands()[2].name, testH.name);
    EXPECT_EQ(reqR.getCommands()[2].operation, testH.operation);
    EXPECT_EQ(reqR.getCommands()[2].value, testH.value);

    WinCCRequest reqW("TESTI,WRITE,7\n");
    EXPECT_EQ(reqW.getCommands().size(), 1);
    EXPECT_EQ(reqW.getCommands()[0].name, "TESTI");
    EXPECT_EQ(reqW.getCommands()[0].operation, WinCCRequest::Command::Operation::Write);
    EXPECT_EQ(reqW.getCommands()[0].value, 7);
}

TEST(WinCCMessageTest, Response) {
    WinCCResponse res;
    res.addParameter("TEST1", {0.7}).addParameter("TEST2", {-123, 27.8, 312.9}).addParameter("TEST3", {});

    EXPECT_EQ(res.getContents(), "TEST1,0.7\nTEST2,-123,27.8,312.9\nTEST3\n");
}

}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}