#include "gtest/gtest.h"

#include "services/CounterRates.h"

namespace
{

TEST(CounterRatesTest, FifoAlfResponse)
{
    CounterRates cr(nullptr, 2, 8);
    auto resultFailure = cr.parseFifoAlfResponse("failure\n");
    EXPECT_TRUE(resultFailure.empty());

    auto resultSingle = cr.parseFifoAlfResponse("success\n0x000123456780badf00d\n0x000123456780badbeef\n");
    vector<vector<uint32_t>> expectedResultSingle = { { 0x0badf00d, 0x0badbeef } };
    EXPECT_EQ(resultSingle, expectedResultSingle);

    auto resultMultiple = cr.parseFifoAlfResponse(
        "success\n"
        "0x000123456780badf00d\n0x000123456780badbeef\n"
        "0x000123456780badcafe\n0x00012345678900df00d\n"
        "0x000123456780badf00d\n0x000123456780badbeef\n");
    vector<vector<uint32_t>> expectedResultMultiple = { { 0x0badf00d, 0x0badbeef }, { 0x0badcafe, 0x900df00d }, { 0x0badf00d, 0x0badbeef } };
    EXPECT_EQ(resultMultiple, expectedResultMultiple);
}

} // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
