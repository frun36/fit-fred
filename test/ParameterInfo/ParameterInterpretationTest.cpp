#include "gtest/gtest.h"
#include "ParameterInfo.h"

namespace {

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

} // namespace

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}