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
        : m_handler(board),
          m_numberOfCounters(board->isTcm() ? 15 : 24),
          m_maxFifoWords(board->isTcm() ? 495 : 480),
          m_names(board->isTcm() ? vector<string>{
                                       "TRIGGER_5_COUNTER",
                                       "TRIGGER_4_COUNTER",
                                       "TRIGGER_2_COUNTER",
                                       "TRIGGER_1_COUNTER",
                                       "TRIGGER_3_COUNTER",
                                       "BACKGROUND_0_COUNTER",
                                       "BACKGROUND_1_COUNTER",
                                       "BACKGROUND_2_COUNTER",
                                       "BACKGROUND_3_COUNTER",
                                       "BACKGROUND_4_COUNTER",
                                       "BACKGROUND_5_COUNTER",
                                       "BACKGROUND_6_COUNTER",
                                       "BACKGROUND_7_COUNTER",
                                       "BACKGROUND_8_COUNTER",
                                       "BACKGROUND_9_COUNTER" }
                                 : vector<string>{ "CH01_CTR_CFD", "CH01_CTR_TRG", "CH02_CTR_CFD", "CH02_CTR_TRG", "CH03_CTR_CFD", "CH03_CTR_TRG", "CH04_CTR_CFD", "CH04_CTR_TRG", "CH05_CTR_CFD", "CH05_CTR_TRG", "CH06_CTR_CFD", "CH06_CTR_TRG", "CH07_CTR_CFD", "CH07_CTR_TRG", "CH08_CTR_CFD", "CH08_CTR_TRG", "CH09_CTR_CFD", "CH09_CTR_TRG", "CH10_CTR_CFD", "CH10_CTR_TRG", "CH11_CTR_CFD", "CH11_CTR_TRG", "CH12_CTR_CFD", "CH12_CTR_TRG" })
    {
    }

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
    const vector<string> m_names;
    optional<vector<uint32_t>> m_counters;
    double m_readInterval;
    optional<vector<double>> m_rates;

    static double mapReadIntervalCodeToSeconds(int64_t code);
    ReadIntervalState handleReadInterval();

    optional<uint32_t> getFifoLoad();
    FifoState evaluateFifoState(uint32_t fifoLoad) const;

    FifoReadResult handleCounterValues(const BoardCommunicationHandler::FifoResponse&& fifoResult, bool clearOnly);
    inline FifoReadResult readFifo(uint32_t fifoLoad, bool clearOnly = false) { return handleCounterValues(move(BasicFitIndefiniteMapi::readFifo(m_handler, "COUNTERS_VALUES_READOUT", fifoLoad)), clearOnly); }
    inline FifoReadResult clearFifo(uint32_t fifoLoad) { return readFifo(fifoLoad, true); }
    vector<uint32_t> readDirectly();
    void resetService();
    bool resetCounters();

    string generateResponse(ReadIntervalState readIntervalState, FifoState fifoState, uint32_t fifoLoad, FifoReadResult fifoReadResult) const;

    optional<string> handleDirectReadout();
    optional<string> handleFifoReadout(ReadIntervalState readIntervalState);

    void processExecution() override;
};