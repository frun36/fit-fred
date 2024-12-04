#pragma once

#include <sstream>
#include <chrono>
#include "communication-utils/AlfResponseParser.h"
#include "BoardCommunicationHandler.h"

#include "services/LoopingFitIndefiniteMapi.h"
#include "TCM.h"
#include "PM.h"

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

        ReadoutResult(
            ReadIntervalState readIntervalState,
            double readInterval,
            FifoState fifoState,
            uint32_t fifoLoad,
            FifoReadResult fifoReadResult,
            const optional<vector<uint32_t>>& counters,
            const optional<vector<double>>& rates) : readIntervalState(readIntervalState),
                                                     readInterval(readInterval),
                                                     fifoState(fifoState),
                                                     fifoLoad(fifoLoad),
                                                     fifoReadResult(fifoReadResult),
                                                     counters(counters),
                                                     rates(rates) {}

        string getString() const;
    };

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

    optional<ReadoutResult> handleDirectReadout();
    optional<ReadoutResult> handleFifoReadout(ReadIntervalState readIntervalState);

    void processExecution() override;
};