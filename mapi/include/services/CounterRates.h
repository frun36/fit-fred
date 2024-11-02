#pragma once

#include "communication-utils/AlfResponseParser.h"
#include "BasicRequestHandler.h"

#ifdef FIT_UNIT_TEST

#include "../../test/mocks/include/mapi.h"
#include "gtest/gtest.h"

namespace
{
class CounterRatesTest_FifoAlfResponse_Test;
class CounterRatesTest_HandleCounterValues_Test;
} // namespace

#else

#include "Fred/Mapi/indefinitemapi.h"

#endif

class CounterRates : public BasicRequestHandler, public IndefiniteMapi
{
#ifdef FIT_UNIT_TEST
    FRIEND_TEST(::CounterRatesTest, FifoAlfResponse);
    FRIEND_TEST(::CounterRatesTest, HandleCounterValues);
#endif

   public:
    CounterRates(shared_ptr<Board> board, uint32_t numberOfCounters, uint32_t maxFifoWords)
        : BasicRequestHandler(board), IndefiniteMapi(), m_numberOfCounters(numberOfCounters), m_maxFifoWords(maxFifoWords) {}

    enum class UpdateRateState {
        Invalid,
        Changed,
        Ok
    };

    enum class FifoState {
        Empty,
        Single,
        Multiple,
        Full,
        Unexpected
    };

    enum class FifoReadResult {
        Failure,
        SuccessNoRates,
        SuccessCleared,
        Success,
        NotPerformed
    };

    class Response {
    private:
        string m_msg;
        bool m_isError;
    public:
        Response(string msg = "", bool isError = false) : m_msg(msg), m_isError(isError) {}

        Response& addUpdateRateChanged();
        Response& addFifoState(FifoState fifoState);
        Response& addFifoReadResult(FifoReadResult fifoReadResult);
        Response& addRatesResponse(string ratesResponse);

        bool isError() const {
            return m_isError;
        }

        operator string() const {
            return m_msg;
        }
    };

    uint32_t m_numberOfCounters;
    uint32_t m_maxFifoWords;
    optional<vector<uint32_t>> m_oldCounters;
    double m_updateRateSeconds;
    optional<vector<double>> m_counterRates;

    static double mapUpdateRateCodeToSeconds(uint8_t code);
    UpdateRateState handleUpdateRate();

    optional<uint32_t> getFifoLoad();
    FifoState evaluateFifoState(uint32_t fifoLoad) const;

    vector<vector<uint32_t>> parseFifoAlfResponse(string alfResponse) const;
    FifoReadResult handleCounterValues(const vector<vector<uint32_t>>& counterValues, bool clearOnly);
    FifoReadResult readFifo(uint32_t fifoLoad, bool clearOnly = false);
    inline FifoReadResult clearFifo(uint32_t fifoLoad) { return readFifo(fifoLoad, true); }
    void resetService();

    string generateRatesResponse() const;

    void processExecution() override;
};