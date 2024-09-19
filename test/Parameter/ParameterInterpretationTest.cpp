#include "gtest/gtest.h"
#include "Parameter.h"

namespace {

TEST(ParameterInterpretationTest, Unsigned) {
    Parameter p(
        "UNSIGNED_TEST",
        0x0,
        4,
        7,
        1,
        Parameter::ValueEncoding::Unsigned,
        0.,
        0.,
        7.,
        false,
        false
    );

    double physicalValue = p.getPhysicalValue(0xffffffff);
    EXPECT_EQ(physicalValue, 1785);

    uint32_t rawValue = p.getRawValue(1785);
    EXPECT_EQ(rawValue, 0x00000ff0);
}

} // namespace

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}