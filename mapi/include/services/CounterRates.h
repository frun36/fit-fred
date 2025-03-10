#pragma once

#include "board/BoardCommunicationHandler.h"

#include "services/templates/LoopingFitIndefiniteMapi.h"

#ifdef FIT_UNIT_TEST

#include "gtest/gtest.h"

namespace
{
class CounterRatesTest_FifoAlfResponse_Test;
class CounterRatesTest_HandleCounterValues_Test;
class CounterRatesTest_Response_Test;
} // namespace

#endif

class CounterRates : public LoopingFitIndefiniteMapi
{
#ifdef FIT_UNIT_TEST
    FRIEND_TEST(::CounterRatesTest, FifoAlfResponse);
    FRIEND_TEST(::CounterRatesTest, HandleCounterValues);
    FRIEND_TEST(::CounterRatesTest, Response);
#endif

   public:
    CounterRates(shared_ptr<Board> board);

   private:
    enum class ReadIntervalState {
        Disabled,
        Unknown,
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
        Partial
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

    struct ReadoutResult {
        ReadIntervalState readIntervalState;
        double readInterval;
        FifoState fifoState;
        uint32_t fifoLoad;
        FifoReadResult fifoReadResult;

        optional<vector<uint32_t>> counters;
        optional<vector<double>> rates;

        useconds_t prevElapsed;

        ReadoutResult(
            ReadIntervalState readIntervalState,
            double readInterval,
            FifoState fifoState,
            uint32_t fifoLoad,
            FifoReadResult fifoReadResult,
            const optional<vector<uint32_t>>& counters,
            const optional<vector<double>>& rates,
            useconds_t prevElapsed) : readIntervalState(readIntervalState),
                                      readInterval(readInterval),
                                      fifoState(fifoState),
                                      fifoLoad(fifoLoad),
                                      fifoReadResult(fifoReadResult),
                                      counters(counters),
                                      rates(rates),
                                      prevElapsed(prevElapsed) {}

        string getString() const;
    };

    BoardCommunicationHandler m_handler;
    const uint32_t m_numberOfCounters;
    const uint32_t m_maxFifoWords;
    const string m_resetFlagName;
    const string m_fifoName;
    const string m_fifoLoadName;
    const vector<string> m_names;
    optional<vector<uint32_t>> m_counters;
    uint32_t m_readIntervalCode;
    double m_readInterval;
    optional<vector<double>> m_rates;

    static double mapReadIntervalCodeToSeconds(int64_t code);
    ReadIntervalState handleReadInterval();

    optional<uint32_t> getFifoLoad();
    FifoState evaluateFifoState(uint32_t fifoLoad) const;

    FifoReadResult handleCounterValues(const BoardCommunicationHandler::FifoResponse&& fifoResult, bool clearOnly);
    inline FifoReadResult readFifo(uint32_t fifoLoad, bool clearOnly = false) { return handleCounterValues(std::move(BasicFitIndefiniteMapi::readFifo(m_handler, m_fifoName, fifoLoad)), clearOnly); }
    inline FifoReadResult clearFifo(uint32_t fifoLoad) { return readFifo(fifoLoad, true); }
    vector<uint32_t> readDirectly();
    void resetService();
    Result<string, string> resetCounters();

    optional<ReadoutResult> handleDirectReadout();
    optional<ReadoutResult> handleFifoReadout(ReadIntervalState readIntervalState);

    void processExecution() override;
};
