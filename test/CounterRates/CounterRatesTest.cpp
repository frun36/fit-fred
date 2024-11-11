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

TEST(CounterRatesTest, HandleCounterValues)
{
    CounterRates cr(nullptr, 2, 8);
    cr.m_readInterval = 2.;
    EXPECT_EQ(cr.m_oldCounters, nullopt);
    EXPECT_EQ(cr.m_counterRates, nullopt);

    cr.handleCounterValues({}, false);
    EXPECT_EQ(cr.m_oldCounters, nullopt);
    EXPECT_EQ(cr.m_counterRates, nullopt);

    cr.handleCounterValues({{1000, 1000}}, false);
    EXPECT_EQ(cr.m_oldCounters, vector<uint32_t>({1000, 1000}));
    EXPECT_EQ(cr.m_counterRates, nullopt);

    cr.handleCounterValues({{3000, 2000}}, false);
    EXPECT_EQ(cr.m_oldCounters, vector<uint32_t>({3000, 2000}));
    EXPECT_EQ(cr.m_counterRates, vector<double>({1000., 500.}));
    
    cr.handleCounterValues({}, false);
    EXPECT_EQ(cr.m_oldCounters, vector<uint32_t>({3000, 2000}));
    EXPECT_EQ(cr.m_counterRates, vector<double>({1000., 500.}));

    cr.handleCounterValues({{4000, 3000}, {6000, 5000}}, false);
    EXPECT_EQ(cr.m_oldCounters, vector<uint32_t>({6000, 5000}));
    EXPECT_EQ(cr.m_counterRates, vector<double>({1000, 1000}));

    cr.handleCounterValues({{7000, 7000}, {8000, 8000}}, true);
    EXPECT_EQ(cr.m_oldCounters, nullopt);
    EXPECT_EQ(cr.m_counterRates, nullopt);

    cr.handleCounterValues({{1000, 1000}, {2000, 2000}}, false);
    EXPECT_EQ(cr.m_oldCounters, vector<uint32_t>({2000, 2000}));
    EXPECT_EQ(cr.m_counterRates, vector<double>({500, 500}));
}

} // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
