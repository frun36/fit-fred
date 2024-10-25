#pragma once

#include "communication-utils/AlfResponseParser.h"
#include "BasicRequestHandler.h"

#ifdef FIT_UNIT_TEST

#include "../../test/mocks/include/mapi.h"
#include "gtest/gtest.h"

namespace
{
class CounterRatesTest_FifoAlfResponse_Test;
} // namespace

#else

#include "Fred/Mapi/indefinitemapi.h"

#endif

class CounterRates : public BasicRequestHandler, public IndefiniteMapi
{
#ifdef FIT_UNIT_TEST
    FRIEND_TEST(::CounterRatesTest, FifoAlfResponse);
#endif

   public:
    CounterRates(shared_ptr<Board> board, uint32_t numberOfCounters, uint32_t maxFifoWords)
        : BasicRequestHandler(board), IndefiniteMapi(), m_numberOfCounters(numberOfCounters), m_maxFifoWords(maxFifoWords) {}

    enum class FifoState {
        Empty,
        Single,
        Multiple,
        Full,
        Unexpected
    };

    enum class FifoReadResult {
        Failed,
        SuccessNoRates,
        Success
    };

    uint32_t m_numberOfCounters;
    uint32_t m_maxFifoWords;
    optional<vector<uint32_t>> m_oldCounters;
    double m_counterUpdateRate;
    optional<vector<double>> m_counterRates;

    optional<uint32_t> getFifoLoad();
    FifoState evaluateFifoState(uint32_t fifoLoad);

    vector<vector<uint32_t>> parseFifoAlfResponse(string alfResponse);
    FifoReadResult readFifo(uint32_t fifoLoad, bool clearOnly = false);

    void processExecution() override;
};