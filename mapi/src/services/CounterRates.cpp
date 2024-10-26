#include "services/CounterRates.h"

optional<uint32_t> CounterRates::getFifoLoad()
{
    SwtSequence seq = processMessageFromWinCC(WinCCRequest::readRequest("CTR_FIFO_LOAD"));
    string alfResponse = executeAlfSequence(seq.getSequence());
    AlfResponseParser parser(alfResponse);
    if (!parser.isSuccess())
        return nullopt;
    AlfResponseParser::Line line = *parser.begin();
    return line.frame.data;
}

CounterRates::FifoState CounterRates::evaluateFifoState(uint32_t fifoLoad) const
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

vector<vector<uint32_t>> CounterRates::parseFifoAlfResponse(string alfResponse) const
{
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

CounterRates::FifoReadResult CounterRates::readFifo(uint32_t fifoLoad, bool clearOnly)
{
    string request;
    for (uint32_t i = 0; i < fifoLoad; i++)
        WinCCRequest::appendToRequest(request, WinCCRequest::readRequest("CTR_FIFO"));
    SwtSequence seq = processMessageFromWinCC(request);
    string alfResponse = executeAlfSequence(seq.getSequence());

    vector<vector<uint32_t>> counterValues = parseFifoAlfResponse(alfResponse);
    if (counterValues.empty())
        return FifoReadResult::Failure;

    if (clearOnly)
        return FifoReadResult::SuccessCleared;
    
    if (counterValues.size() == 1 && !m_oldCounters.has_value()) {
        m_oldCounters = counterValues[0];
        return FifoReadResult::SuccessNoRates;
    } else {
        size_t counterValuesSize = counterValues.size();
        const vector<uint32_t>& newValues = counterValues.back();
        const vector<uint32_t>& oldValues = (counterValuesSize > 1) ? counterValues[counterValuesSize - 2] : *m_oldCounters;
        m_counterRates = vector<double>(m_numberOfCounters);
        for (size_t i = 0; i < m_numberOfCounters; i++)
            (*m_counterRates)[i] = (oldValues[i] - newValues[i]) / m_counterUpdateRate;
        return FifoReadResult::Success;
    }
}

void CounterRates::processExecution()
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

    FifoReadResult readResult;
    switch (evaluateFifoState(*fifoLoad)) {
        case FifoState::Unexpected:
            publishError("Unexpected FIFO state");
            return;
        case FifoState::Empty:
            publishAnswer("No counter data");
            return;
        case FifoState::Multiple:
            publishError("Warning: multiple sets of counters in FIFO");
            [[fallthrough]];
        case FifoState::Single:
            readResult = readFifo(*fifoLoad);
            break;
        case FifoState::Full:
            publishError("FIFO is full, clearing");
            readResult = clearFifo(*fifoLoad);
            break;
    }

    switch (readResult) {
        case FifoReadResult::Success:
            publishAnswer(generateResponse());
            break;
        case FifoReadResult::SuccessNoRates:
            publishAnswer("No rate data for first counter measurement");
            break;
        case FifoReadResult::SuccessCleared:
            publishAnswer("No data: FIFO cleared successfully");
            break;
        case FifoReadResult::Failure:
            publishError("Couldn't read FIFO");
            break;
    }
}

string CounterRates::generateResponse() const {
    if (!m_counterRates.has_value())
        return "Unexpected: no counter data";
    
    stringstream ss;
    for (auto rate : *m_counterRates) {
        ss << rate << '\n';
    }

    return ss.str();
}