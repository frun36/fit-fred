#include "services/CounterRates.h"

optional<uint32_t> CounterRates::getFifoLoad()
{
    SwtSequence seq = m_handler.processMessageFromWinCC(WinCCRequest::readRequest("CTR_FIFO_LOAD"));
    string alfResponse = executeAlfSequence(seq.getSequence());
    AlfResponseParser parser(alfResponse);
    if (!parser.isSuccess())
        return nullopt;
    AlfResponseParser::Line line = *parser.begin();
    return line.frame.data;
}

double CounterRates::mapUpdateRateCodeToSeconds(uint8_t code)
{
    switch (code) {
        case 0:
            return 0.;
        case 1:
            return 0.1;
        case 2:
            return 0.2;
        case 3:
            return 0.5;
        case 4:
            return 1.0;
        case 5:
            return 2.0;
        case 6:
            return 5.0;
        case 7:
            return 10.0;
        default:
            return NAN;
    }
}

void CounterRates::resetService()
{
    m_oldCounters = nullopt;
    m_counterRates = nullopt;
}

CounterRates::UpdateRateState CounterRates::handleUpdateRate()
{
    optional<uint8_t> currUpdateRateCode = m_handler.getBoard()->at("COUNTER_UPD_RATE").getStoredValueOptional();
    double currUpdateRateSeconds = mapUpdateRateCodeToSeconds(*currUpdateRateCode);

    if (!currUpdateRateCode.has_value() || *currUpdateRateCode < 1 || *currUpdateRateCode > 7) {
        return UpdateRateState::Invalid;
    } else if (m_updateRateSeconds != currUpdateRateSeconds) {
        m_updateRateSeconds = currUpdateRateSeconds;
        resetService();
        return UpdateRateState::Changed;
    } else {
        return UpdateRateState::Ok;
    }
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

CounterRates::FifoReadResult CounterRates::handleCounterValues(const vector<vector<uint32_t>>& counterValues, bool clearOnly)
{
    if (counterValues.empty())
        return FifoReadResult::Failure;

    if (clearOnly) {
        resetService();
        return FifoReadResult::SuccessCleared;
    }

    if (counterValues.size() == 1 && !m_oldCounters.has_value()) {
        m_oldCounters = counterValues[0];
        return FifoReadResult::SuccessNoRates;
    } else {
        size_t counterValuesSize = counterValues.size();
        const vector<uint32_t>& newValues = counterValues.back();
        const vector<uint32_t>& oldValues = (counterValuesSize > 1) ? counterValues[counterValuesSize - 2] : *m_oldCounters;
        if (!m_counterRates.has_value())
            m_counterRates = vector<double>(m_numberOfCounters);
        for (size_t i = 0; i < m_numberOfCounters; i++)
            m_counterRates->at(i) = (newValues[i] - oldValues[i]) / m_updateRateSeconds;
        m_oldCounters = newValues;
        return FifoReadResult::Success;
    }
}

CounterRates::FifoReadResult CounterRates::readFifo(uint32_t fifoLoad, bool clearOnly)
{
    string request;
    for (uint32_t i = 0; i < fifoLoad; i++)
        WinCCRequest::appendToRequest(request, WinCCRequest::readRequest("CTR_FIFO"));
    SwtSequence seq = m_handler.processMessageFromWinCC(request);
    string alfResponse = executeAlfSequence(seq.getSequence());

    vector<vector<uint32_t>> counterValues = parseFifoAlfResponse(alfResponse);
    return handleCounterValues(counterValues, clearOnly);
}

void CounterRates::processExecution()
{
    bool running = true;

    string request = waitForRequest(running);
    if (running == false) {
        return;
    }

#ifndef FIT_UNIT_TEST
    UpdateRateState updateRateState = handleUpdateRate();
#else
    UpdateRateState updateRateState = UpdateRateState::Ok;
#endif

    if (updateRateState == UpdateRateState::Invalid) {
        publishError("Invalid update rate");
        return;
    }

    optional<uint32_t> fifoLoad = getFifoLoad();
    if (!fifoLoad.has_value()) {
        publishError("Couldn't read FIFO state");
        return;
    }
    FifoState fifoState = evaluateFifoState(*fifoLoad);

    Response response;

    if (updateRateState == UpdateRateState::Changed) {
        fifoState = FifoState::Full;
        response.addUpdateRateChanged();
    }

    response.addFifoState(fifoState);

    FifoReadResult readResult = FifoReadResult::NotPerformed;
    if (fifoState == FifoState::Single || fifoState == FifoState::Multiple) {
        readResult = readFifo(*fifoLoad);
    } else if (fifoState == FifoState::Full) {
        readResult = clearFifo(*fifoLoad);
    }
    
    response.addFifoReadResult(readResult);

    if (readResult == FifoReadResult::Success)
        response.addRatesResponse(generateRatesResponse());

    if (response.isError())
        publishError(response);
    else
        publishAnswer(response);
}

CounterRates::Response& CounterRates::Response::addUpdateRateChanged() {
    m_msg += "Update rate changed\n";
    return *this;
}

CounterRates::Response& CounterRates::Response::addFifoState(FifoState fifoState) {
    switch (fifoState) {
        case FifoState::Unexpected:
            m_msg += "FIFO state: unexpected\n";
            m_isError = true;
            break;
        case FifoState::Empty:
            m_msg += "FIFO state: empty\n";
            break;
        case FifoState::Multiple:
            m_msg += "FIFO state: warning - multiple counter sets in FIFO\n";
            break;
        case FifoState::Single:
            m_msg += "FIFO state: ok\n";
            break;
        case FifoState::Full:
            m_msg += "FIFO state: full\n";
            break;
    }
    return *this;
}

CounterRates::Response& CounterRates::Response::addFifoReadResult(FifoReadResult fifoReadResult) {
    switch (fifoReadResult) {
        case FifoReadResult::Success:
            m_msg += "FIFO read: successful\n";
            break;
        case FifoReadResult::SuccessNoRates:
            m_msg += "FIFO read: successful, no rates\n";
            break;
        case FifoReadResult::SuccessCleared:
            m_msg += "FIFO read: successful, cleared\n";
            break;
        case FifoReadResult::Failure:
            m_isError = true;
            m_msg += "FIFO read: failed\n";
            break;
        case FifoReadResult::NotPerformed:
            m_msg += "FIFO read: not performed\n";
            break;
    }

    return *this;
}

CounterRates::Response& CounterRates::Response::addRatesResponse(string ratesResponse) {
    m_msg += ratesResponse;
    return *this;
}

string CounterRates::generateRatesResponse() const
{
    if (!m_counterRates.has_value())
        return "Unexpected: no counter data";

    stringstream ss;
    for (auto rate : *m_counterRates) {
        ss << rate << '\n';
    }

    return ss.str();
}