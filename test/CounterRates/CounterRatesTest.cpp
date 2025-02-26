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
    EXPECT_EQ(cr.m_counters, nullopt);
    EXPECT_EQ(cr.m_rates, nullopt);

    cr.handleCounterValues({}, false);
    EXPECT_EQ(cr.m_counters, nullopt);
    EXPECT_EQ(cr.m_rates, nullopt);

    cr.handleCounterValues({{1000, 1000}}, false);
    EXPECT_EQ(cr.m_counters, vector<uint32_t>({1000, 1000}));
    EXPECT_EQ(cr.m_rates, nullopt);

    cr.handleCounterValues({{3000, 2000}}, false);
    EXPECT_EQ(cr.m_counters, vector<uint32_t>({3000, 2000}));
    EXPECT_EQ(cr.m_rates, vector<double>({1000., 500.}));
    
    cr.handleCounterValues({}, false);
    EXPECT_EQ(cr.m_counters, vector<uint32_t>({3000, 2000}));
    EXPECT_EQ(cr.m_rates, vector<double>({1000., 500.}));

    cr.handleCounterValues({{4000, 3000}, {6000, 5000}}, false);
    EXPECT_EQ(cr.m_counters, vector<uint32_t>({6000, 5000}));
    EXPECT_EQ(cr.m_rates, vector<double>({1000, 1000}));

    cr.handleCounterValues({{7000, 7000}, {8000, 8000}}, true);
    EXPECT_EQ(cr.m_counters, nullopt);
    EXPECT_EQ(cr.m_rates, nullopt);

    cr.handleCounterValues({{1000, 1000}, {2000, 2000}}, false);
    EXPECT_EQ(cr.m_counters, vector<uint32_t>({2000, 2000}));
    EXPECT_EQ(cr.m_rates, vector<double>({500, 500}));
}

// Just to test overall formatting - assumes correct conversion of enum types
TEST(CounterRatesTest, Response) {
    CounterRates cr(nullptr, 2, 8);
    cr.m_readInterval = 0.5;

    cr.m_counters = nullopt;
    cr.m_rates = nullopt;
    string responseNone = cr.generateResponse(CounterRates::ReadIntervalState::Ok, CounterRates::FifoState::Single, 12, CounterRates::FifoReadResult::Success);
    EXPECT_EQ(responseNone, "READ_INTERVAL,OK,0.5s\nFIFO_STATE,SINGLE,12\nFIFO_READ_RESULT,SUCCESS\nCOUNTERS,-\nRATES,-");
    
    cr.m_counters = vector<uint32_t>({123, 456});
    cr.m_rates = vector<double>({1.2, 2.1});
    string responseSome = cr.generateResponse(CounterRates::ReadIntervalState::Ok, CounterRates::FifoState::Single, 21, CounterRates::FifoReadResult::Success);
    EXPECT_EQ(responseSome, "READ_INTERVAL,OK,0.5s\nFIFO_STATE,SINGLE,21\nFIFO_READ_RESULT,SUCCESS\nCOUNTERS,123,456\nRATES,1.2,2.1");
}

} // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
