#include "gtest/gtest.h"
#include "Parameters.h"
#include "ParameterInfo.h"
#include <unordered_map>

namespace {

unordered_map<string, ParameterInfo> testMap = {
    {"SIGNED_REG",      ParameterInfo("SIGNED_REG",     0x0,    0,  31, 1,  ParameterInfo::ValueEncoding::Signed32, 0,      100,    1,  false,  false) },
    {"SIGNED_HALF",     ParameterInfo("SIGNED_HALF",    0x1,    0,  15, 1,  ParameterInfo::ValueEncoding::Signed16, -100,   100,    1,  false,  false) },
    {"UNSIGNED_HALF",   ParameterInfo("UNSIGNED_HALF",  0x1,    16, 31, 1,  ParameterInfo::ValueEncoding::Unsigned, 0,      100,    1,  false,  false) },
    {"READONLY_FLAG",   ParameterInfo("READONLY_FLAG",  0x2,    7,  7,  1,  ParameterInfo::ValueEncoding::Unsigned, 0,      1,      1,  false,  true) },
};

TEST(ParametersTest, PIM) {
    Parameters p(testMap);

    string signedRegReadReq = p.processInputMessage("SIGNED_REG,READ");
    EXPECT_EQ(signedRegReadReq, "reset\n0x0000000000000000000,write\nread\n");

    string signedRegReadRes = p.processOutputMessage("success\n0\n0x00000000000FFFFFFFF");
    EXPECT_EQ(signedRegReadRes, "SIGNED_REG,-1\n");

    string unsignedHalfReadSignedHalfWriteReq = p.processInputMessage("UNSIGNED_HALF,READ\nSIGNED_HALF,WRITE,-12");
    EXPECT_EQ(unsignedHalfReadSignedHalfWriteReq, "reset\n0x0000000000100000000,write\nread\n0x00200000001FFFF0000,write\n0x003000000010000FFF4,write\n0x0000000000100000000,write\nread\n");
    // string unsignedHalfReadSignedHalfWriteRes = p.processOutputMessage("success\n0\n0x00000000001aaaa0007\n0x");
    
}

}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
