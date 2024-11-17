#pragma once

#include <sstream>
#include "communication-utils/AlfResponseParser.h"
#include "BoardCommunicationHandler.h"

#include "services/BasicFitIndefiniteMapi.h"

#ifdef FIT_UNIT_TEST

#include "gtest/gtest.h"

namespace
{
class CounterRatesTest_FifoAlfResponse_Test;
class CounterRatesTest_HandleCounterValues_Test;
class CounterRatesTest_Response_Test;
} // namespace

#endif

class CounterRates : public BasicFitIndefiniteMapi
{
#ifdef FIT_UNIT_TEST
    FRIEND_TEST(::CounterRatesTest, FifoAlfResponse);
    FRIEND_TEST(::CounterRatesTest, HandleCounterValues);
    FRIEND_TEST(::CounterRatesTest, Response);
#endif

   public:
    CounterRates(shared_ptr<Board> board)
        : m_handler(board), m_numberOfCounters(board->isTcm() ? 15 : 24), m_maxFifoWords(board->isTcm() ? 495 : 480) {}

    enum class ReadIntervalState {
        Disabled,
        Invalid,
        Changed,
        Ok
    };

    friend ostream& operator<<(ostream& os, ReadIntervalState readIntervalState);

    enum class FifoState {
        BoardError,
        Empty,
        Single,
        Multiple,
        Outdated,
        Unexpected
    };

    friend ostream& operator<<(ostream& os, FifoState readIntervalState);

    enum class FifoReadResult {
        Failure,
        SuccessNoRates,
        SuccessCleared,
        Success,
        NotPerformed
    };

    friend ostream& operator<<(ostream& os, FifoReadResult readIntervalState);

   private:
    BoardCommunicationHandler m_handler;
    const uint32_t m_numberOfCounters;
    const uint32_t m_maxFifoWords;
    optional<vector<uint32_t>> m_counters;
    double m_readInterval;
    optional<vector<double>> m_rates;

    static double mapReadIntervalCodeToSeconds(int64_t code);
    ReadIntervalState handleReadInterval();

    optional<uint32_t> getFifoLoad();
    FifoState evaluateFifoState(uint32_t fifoLoad) const;

    FifoReadResult handleCounterValues(const vector<vector<uint32_t>>&& counterValues, bool clearOnly);
    FifoReadResult readFifo(uint32_t fifoLoad, bool clearOnly = false);
    inline FifoReadResult clearFifo(uint32_t fifoLoad) { return readFifo(fifoLoad, true); }
    void resetService();

    string generateResponse(ReadIntervalState readIntervalState, FifoState fifoState, uint32_t fifoLoad, FifoReadResult fifoReadResult) const;

    void processExecution() override;
};