#include "gtest/gtest.h"
#include "ParameterInfo.h"

namespace {

TEST(ParameterInterpretationTest, Encoding) {
    EXPECT_EQ(ParameterInfo::fromNBitSigned(0x0000ffef, 16), -17.);
    EXPECT_EQ(ParameterInfo::asNBitSigned(-17., 12), 0x00000fef);
    EXPECT_EQ(ParameterInfo::fromNBitSigned(0x8000DAA7, 32), -2147427673.);
    EXPECT_EQ(ParameterInfo::asNBitSigned(-8., 4), 0x00000008);
}

TEST(ParameterInterpretationTest, Unsigned) {
    ParameterInfo p(
        "UNSIGNED_TEST",
        0x0,
        4,
        7,
        1,
        ParameterInfo::ValueEncoding::Unsigned,
        0.,
        0.,
        7.,
        false,
        false
    );

    double physicalValue = p.getPhysicalValue(0xffffffff);
    EXPECT_EQ(physicalValue, 105);

    uint32_t rawValue = p.getRawValue(105);
    EXPECT_EQ(rawValue, 0x000000f0);
}

TEST(ParameterInterpretationTest, Signed32) {
    ParameterInfo p(
        "SIGNED32_TEST",
        0x1,
        0,
        31,
        1,
        ParameterInfo::ValueEncoding::Signed,
        0.,
        0.,
        -2.,
        false,
        false
    );

    double physicalValue = p.getPhysicalValue(0x8000DAA7);
    EXPECT_EQ(physicalValue, 4294855346.);

    uint32_t rawValue = p.getRawValue(4294855346.);
    EXPECT_EQ(rawValue, 0x8000DAA7);
}

TEST(ParameterInterpretationTest, Signed16) {
    ParameterInfo p(
        "SIGNED16_TEST",
        0x2,
        12,
        27,
        1,
        ParameterInfo::ValueEncoding::Signed,
        0.,
        0.,
        3.,
        false,
        false
    );

    double physicalValue = p.getPhysicalValue(0xFFFFFFFF);
    EXPECT_EQ(physicalValue, -3.);

    uint32_t rawValue = p.getRawValue(-3.);
    EXPECT_EQ(rawValue, 0x0FFFF000);
}

} // namespace

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}