#include "gtest/gtest.h"

#include "services/CounterRates.h"

namespace
{

TEST(CounterRatesTest, FifoAlfResponse)
{
    CounterRates(nullptr, 1, 1);
}

} // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
