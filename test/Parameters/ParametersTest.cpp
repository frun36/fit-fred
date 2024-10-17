#include "gtest/gtest.h"
#include "Parameters.h"
#include "ParameterInfo.h"
#include "SwtSequence.h"
#include <unordered_map>

namespace
{

unordered_map<string, ParameterInfo> testMap = {
    { "SIGNED_REG", ParameterInfo("SIGNED_REG", 0x0, 0, 32, 1, ParameterInfo::ValueEncoding::Signed, 0, 100, 1, false, false) },
    { "SIGNED_HALF", ParameterInfo("SIGNED_HALF", 0x1, 0, 16, 1, ParameterInfo::ValueEncoding::Signed, -100, 100, 1, false, false) },
    { "UNSIGNED_HALF", ParameterInfo("UNSIGNED_HALF", 0x1, 16, 16, 1, ParameterInfo::ValueEncoding::Unsigned, 0, 100, 1, false, false) },
    { "READONLY_FLAG", ParameterInfo("READONLY_FLAG", 0x0, 7, 1, 1, ParameterInfo::ValueEncoding::Unsigned, 0, 1, 1, false, true) },
};

TEST(ParametersTest, SignedRegRead)
{
    Parameters p(testMap);

    string signedRegReadReq = p.processInputMessage("SIGNED_REG,READ");
    string expectedSignedRegReadReq = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x0, {}, true) }).getSequence();
    EXPECT_EQ(signedRegReadReq, expectedSignedRegReadReq);

    string signedRegReadRes = p.processOutputMessage("success\n0\n0x00000000000FFFFFFFF");
    EXPECT_EQ(signedRegReadRes, "SIGNED_REG,-1\n");
}

TEST(ParametersTest, Halves)
{
    Parameters p(testMap);

    string unsignedHalfReadReq = p.processInputMessage("UNSIGNED_HALF,READ\n");
    string expectedUnsignedHalfReadReq = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true) }).getSequence();
    EXPECT_EQ(unsignedHalfReadReq, expectedUnsignedHalfReadReq);

    string unsignedHalfReadRes = p.processOutputMessage("success\n0\n0x00000000001aaaa0007");
    EXPECT_EQ(unsignedHalfReadRes, "UNSIGNED_HALF,43690\n");

    string signedHalfWriteReq = p.processInputMessage("SIGNED_HALF,WRITE,-12");
    string expectedSignedHalfWriteReq = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x1, { 0xffff0000, 0x0000fff4 }, false),
                                                      SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true) })
                                            .getSequence();
    EXPECT_EQ(signedHalfWriteReq, expectedSignedHalfWriteReq);

    string signedHalfWriteRes = p.processOutputMessage("success\n0\n0\n0x00000000001aaaafff4");
    EXPECT_EQ(signedHalfWriteRes, "SIGNED_HALF,-12\n");

    string bothReadReq = p.processInputMessage("SIGNED_HALF,READ\nUNSIGNED_HALF,READ");
    string expectedBothReadReq = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true) }).getSequence();
    EXPECT_EQ(bothReadReq, expectedBothReadReq);

    string bothWriteReq = p.processInputMessage("SIGNED_HALF,WRITE,-21133\nUNSIGNED_HALF,WRITE,0xAD72");
    string expectedBothWriteReq = SwtSequence({ SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x1, { 0x0000ffff, 0xad710000 }, false),
                                                SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, 0x1, { 0xffff0000, 0x0000ad72 }, false),
                                                SwtSequence::SwtOperation(SwtSequence::Operation::Read, 0x1, {}, true) })
                                      .getSequence();
    EXPECT_EQ(bothReadReq, expectedBothReadReq);
}

TEST(ParametersTest, ReadonlyWrite)
{
    Parameters p(testMap);

    string readonlyWriteReq;

    try {
        readonlyWriteReq = p.processInputMessage("READONLY_FLAG,WRITE,1");
    } catch (const runtime_error& e) {
        readonlyWriteReq = e.what();
    }

    EXPECT_EQ(readonlyWriteReq, "READONLY_FLAG: attempted WRITE on read-only parameter");
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace