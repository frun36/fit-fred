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

    optional<uint32_t> getFifoLoad()
    {
        SwtSequence seq = processMessageFromWinCC(WinCCRequest::readRequest("CTR_FIFO_LOAD"));
        string alfResponse = executeAlfSequence(seq.getSequence());
        AlfResponseParser parser(alfResponse);
        if (!parser.isSuccess())
            return nullopt;
        AlfResponseParser::Line line = *parser.begin();
        return line.frame.data;
    }

    FifoState evaluateFifoState(uint32_t fifoLoad)
    {
        if (fifoLoad == 0) {
            return FifoState::Empty;
        } else if (fifoLoad == m_numberOfCounters) {
            return FifoState::Single;
        } else if (fifoLoad < m_maxFifoWords && fifoLoad % m_numberOfCounters == 0) {
            return FifoState::Multiple;
        } else if (fifoLoad >= m_maxFifoWords) {
            return FifoState::Full;
        } else {
            return FifoState::Unexpected;
        }
    }

    vector<vector<uint32_t>> parseFifoAlfResponse(string alfResponse) {
        AlfResponseParser parser(alfResponse);
        if (!parser.isSuccess())
            return {};

        vector<vector<uint32_t>> counterValues;
        uint32_t idx = 0;
        for (auto line : parser) {
            if (idx % m_numberOfCounters == 0)
                counterValues.push_back(vector<uint32_t>(m_numberOfCounters));
            counterValues[idx / m_numberOfCounters][idx % m_numberOfCounters] = line.frame.data;
            idx++;
        }

        return counterValues;
    }

    FifoReadResult readFifo(uint32_t fifoLoad, bool clearOnly = false)
    {
        string request;
        for (uint32_t i = 0; i < fifoLoad; i++)
            WinCCRequest::appendToRequest(request, WinCCRequest::readRequest("CTR_FIFO"));
        SwtSequence seq = processMessageFromWinCC(request);
        string alfResponse = executeAlfSequence(seq.getSequence());

        vector<vector<uint32_t>> counterValues = parseFifoAlfResponse(alfResponse);
        if (counterValues.empty())
            return FifoReadResult::Failed;
        if (counterValues.size() == 1 && !m_oldCounters.has_value()) {
            m_oldCounters = counterValues[0];
            return FifoReadResult::SuccessNoRates;
        } else {
            size_t counterValuesSize = counterValues.size();
            const vector<uint32_t>& newValues = counterValues.back();
            const vector<uint32_t>& oldValues = (counterValuesSize > 1) ? counterValues[counterValuesSize - 2] : oldValues;
            m_counterRates = vector<double>(m_numberOfCounters);
            for (size_t i = 0; i < m_numberOfCounters; i++)
                (*m_counterRates)[i] = (oldValues[i] - newValues[i]) / m_counterUpdateRate;
            return FifoReadResult::Success;
        }
    }

    void processExecution() override
    {
        bool running = true;

        string request = waitForRequest(running);
        if (running == false) {
            return;
        }

        optional<uint32_t> fifoLoad = getFifoLoad();
        if (!fifoLoad.has_value()) {
            publishError("Couldn't read FIFO state");
            return;
        }



        switch (evaluateFifoState(*fifoLoad)) {
            case FifoState::Empty:
                publishError("No data");
                break;
            case FifoState::Multiple:
                publishError("Warning: multiple sets of counters in FIFO");
            [[fallthrough]]
            case FifoState::Single:
                readFifo(*fifoLoad);
                break;
            case FifoState::Full:
                publishError("FIFO is full, clearing");
                readFifo(*fifoLoad, true);
                break;
            case FifoState::Unexpected:
                publishError("Unexpected FIFO state");
                break;
        }
    }
};