#pragma once

#include "BasicFitIndefiniteMapi.h"
#include "AlfResponseParser.h"

class CounterRates : BasicFitIndefiniteMapi
{
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

    uint32_t m_maxFifoWords;
    uint32_t m_numberOfCounters;
    uint32_t m_fifoSize;
    optional<vector<uint32_t>> m_oldCounters;
    double m_counterUpdateRate;
    optional<vector<double>> m_counterRates;

    optional<uint32_t> getFifoLoad();
    FifoState evaluateFifoState(uint32_t fifoLoad);

    vector<vector<uint32_t>> parseFifoAlfResponse(string alfResponse);
    FifoReadResult readFifo(uint32_t fifoLoad, bool clearOnly = false);

    void processExecution() override;
};