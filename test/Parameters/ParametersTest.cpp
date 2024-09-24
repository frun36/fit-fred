#include "gtest/gtest.h"
#include "Parameters.h"
#include "ParameterInfo.h"
#include "SwtSequence.h"
#include <unordered_map>

namespace {

unordered_map<string, ParameterInfo> testMap = {
    {"SIGNED_REG",      ParameterInfo("SIGNED_REG",     0x0,    0,  32, 1,  ParameterInfo::ValueEncoding::Signed, 0,      100,    1,  false,  false) },
    {"SIGNED_HALF",     ParameterInfo("SIGNED_HALF",    0x1,    0,  16, 1,  ParameterInfo::ValueEncoding::Signed, -100,   100,    1,  false,  false) },
    {"UNSIGNED_HALF",   ParameterInfo("UNSIGNED_HALF",  0x1,    16, 16, 1,  ParameterInfo::ValueEncoding::Unsigned, 0,      100,    1,  false,  false) },
    {"READONLY_FLAG",   ParameterInfo("READONLY_FLAG",  0x0,    7,  1,  1,  ParameterInfo::ValueEncoding::Unsigned, 0,      1,      1,  false,  true) },
};

TEST(ParametersTest, SignedRegRead) {
    Parameters p(testMap);

    string signedRegReadReq = p.processInputMessage("SIGNED_REG,READ");
    string expectedSignedRegReadReq = SwtSequence({
        SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x0, {}, true)
    }).getSequence();
    EXPECT_EQ(signedRegReadReq, expectedSignedRegReadReq);

    string signedRegReadRes = p.processOutputMessage("success\n0\n0x00000000000FFFFFFFF");
    EXPECT_EQ(signedRegReadRes, "SIGNED_REG,-1\n");
}

TEST(ParametersTest, UnsignedHalfReadSignedHalfWrite) {
    Parameters p(testMap);

    string unsignedHalfReadSignedHalfWriteReq = p.processInputMessage("UNSIGNED_HALF,READ\nSIGNED_HALF,WRITE,-12");
    string expectedUnsignedHalfReadSignedHalfWriteReq = SwtSequence({
        SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true),
        SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x1, {0xffff0000, 0x0000fff4}, false),
        SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true)
    }).getSequence();

    EXPECT_EQ(unsignedHalfReadSignedHalfWriteReq, expectedUnsignedHalfReadSignedHalfWriteReq);

    string unsignedHalfReadSignedHalfWriteRes = p.processOutputMessage("success\n0\n0x00000000001aaaa0007\n0\n0\n0x00000000001aaaafff4");
    EXPECT_EQ(unsignedHalfReadSignedHalfWriteRes, "UNSIGNED_HALF,43690\nSIGNED_HALF,7\nUNSIGNED_HALF,43690\nSIGNED_HALF,-12\n");   
}

TEST(ParametersTest, ReadonlyWrite) {
    Parameters p(testMap);

    string readonlyWriteReq;

    try {
        readonlyWriteReq = p.processInputMessage("READONLY_FLAG,WRITE,1");
    } catch (const runtime_error& e) {
        readonlyWriteReq = e.what();
    }

    EXPECT_EQ(readonlyWriteReq, "READONLY_FLAG: attempted WRITE on read-only parameter");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

}