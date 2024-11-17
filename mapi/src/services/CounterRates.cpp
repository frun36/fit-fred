#include "services/CounterRates.h"
#include "FREDServer/Alfred/print.h"
#include <unistd.h>

optional<uint32_t> CounterRates::getFifoLoad()
{
    auto parsedResponse = processSequenceThroughHandler(m_handler, WinCCRequest::readRequest("COUNTERS_FIFO_LOAD"));
    if (parsedResponse.isError())
        return nullopt;
    return m_handler.getBoard()->at("COUNTERS_FIFO_LOAD").getElectronicValueOptional();
}

double CounterRates::mapReadIntervalCodeToSeconds(int64_t code)
{
    static constexpr double readIntervalMapping[] = { 0.0, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0 };
    return code >= 0 && code <= 7 ? readIntervalMapping[code] : 0.;
}

void CounterRates::resetService()
{
    m_counters = nullopt;
    m_rates = nullopt;
}

CounterRates::ReadIntervalState CounterRates::handleReadInterval()
{
    optional<int64_t> currReadIntervalCode;
    shared_ptr<Board> board = m_handler.getBoard();
    if (board->isTcm()) {
        currReadIntervalCode = board->at("COUNTER_READ_INTERVAL").getElectronicValueOptional();
        if (!currReadIntervalCode.has_value()) {
            processSequenceThroughHandler(m_handler, "COUNTER_READ_INTERVAL,READ");
            currReadIntervalCode = board->at("COUNTER_READ_INTERVAL").getElectronicValueOptional();
        }
    } else {
        currReadIntervalCode = board->getParentBoard()->at("COUNTER_READ_INTERVAL").getElectronicValueOptional();
    }

    double currReadInterval = mapReadIntervalCodeToSeconds(*currReadIntervalCode);

    if (!currReadIntervalCode.has_value() || *currReadIntervalCode < 0 || *currReadIntervalCode > 7) {
        return ReadIntervalState::Invalid;
    } else if (*currReadIntervalCode == 0) {
        return ReadIntervalState::Disabled;
    } else if (m_readInterval != currReadInterval) {
        m_readInterval = currReadInterval;
        resetService();
        return ReadIntervalState::Changed;
    } else {
        return ReadIntervalState::Ok;
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
        return FifoState::Outdated;
    } else {
        return FifoState::Unexpected;
    }
}

CounterRates::FifoReadResult CounterRates::handleCounterValues(const vector<vector<uint32_t>>&& counterValues, bool clearOnly)
{
    if (counterValues.empty())
        return FifoReadResult::Failure;

    if (clearOnly) {
        resetService();
        return FifoReadResult::SuccessCleared;
    }

    if (counterValues.size() == 1 && !m_counters.has_value()) {
        m_counters = counterValues[0];
        return FifoReadResult::SuccessNoRates;
    } else {
        size_t counterValuesSize = counterValues.size();
        const vector<uint32_t>& newValues = counterValues.back();
        const vector<uint32_t>& oldValues = (counterValuesSize > 1) ? counterValues[counterValuesSize - 2] : *m_counters;
        if (!m_rates.has_value())
            m_rates = vector<double>(m_numberOfCounters);
        for (size_t i = 0; i < m_numberOfCounters; i++)
            m_rates->at(i) = (newValues[i] - oldValues[i]) / m_readInterval;
        m_counters = newValues;
        return FifoReadResult::Success;
    }
}

CounterRates::FifoReadResult CounterRates::readFifo(uint32_t fifoLoad, bool clearOnly)
{
    SwtSequence request = m_handler.createReadFifoRequest("COUNTERS_VALUES_READOUT", fifoLoad);
    string alfResponse = executeAlfSequence(request.getSequence());
    BoardCommunicationHandler::FifoResponse response = m_handler.parseFifo(alfResponse);
    if(response.isError())
        return {};

    return handleCounterValues(move(response.fifoContent), clearOnly);
}

void CounterRates::processExecution()
{
    Print::PrintVerbose("Entering CounterRates process execution");
    usleep(static_cast<useconds_t>(m_readInterval * 0.5 * 1e6));

#ifdef FIT_UNIT_TEST
    ReadIntervalState readIntervalState = ReadIntervalState::Ok;
#else
    ReadIntervalState readIntervalState = handleReadInterval();
#endif

    if (readIntervalState == ReadIntervalState::Invalid) {
        publishError("Invalid read interval");
        return;
    } else if (readIntervalState == ReadIntervalState::Disabled) {
        publishAnswer("COUNTER_READ_INTERVAL = 0: service disabled. Send request to re-enable");
        bool running;
        waitForRequest(running);
        return;
    }

    optional<uint32_t> fifoLoad = getFifoLoad();
    if (!fifoLoad.has_value()) {
        publishError("Couldn't read FIFO state");
        return;
    }

    FifoState fifoState;
    if (readIntervalState == ReadIntervalState::Changed) {
        fifoState = FifoState::Outdated;
    } else {
        fifoState = evaluateFifoState(*fifoLoad);
    }


    FifoReadResult fifoReadResult = FifoReadResult::NotPerformed;
    if (fifoState == FifoState::Single || fifoState == FifoState::Multiple) {
        fifoReadResult = readFifo(*fifoLoad);
    } else if (fifoState == FifoState::Outdated) {
        fifoReadResult = clearFifo(*fifoLoad);
    }

    if (fifoReadResult == FifoReadResult::Failure) {
        publishError("Couldn't read FIFO");
        return;
    }

    string response = generateResponse(readIntervalState, fifoState, *fifoLoad, fifoReadResult);
    Print::PrintVerbose(response);
    publishAnswer(response);
}

ostream& operator<<(ostream& os, CounterRates::ReadIntervalState readIntervalState)
{
    switch (readIntervalState) {
        case CounterRates::ReadIntervalState::Invalid:
            os << "INVALID";
            break;
        case CounterRates::ReadIntervalState::Disabled:
            os << "DISABLED";
            break;
        case CounterRates::ReadIntervalState::Changed:
            os << "CHANGED";
            break;
        case CounterRates::ReadIntervalState::Ok:
            os << "OK";
            break;
    }

    return os;
}

ostream& operator<<(ostream& os, CounterRates::FifoState fifoState)
{
    switch (fifoState) {
        case CounterRates::FifoState::Empty:
            os << "EMPTY";
            break;
        case CounterRates::FifoState::Single:
            os << "SINGLE";
            break;
        case CounterRates::FifoState::Multiple:
            os << "MULTIPLE";
            break;
        case CounterRates::FifoState::Outdated:
            os << "OUTDATED";
            break;
        case CounterRates::FifoState::Unexpected:
            os << "UNEXPECTED";
            break;
    }
    return os;
}

ostream& operator<<(ostream& os, CounterRates::FifoReadResult fifoReadResult)
{
    switch (fifoReadResult) {
        case CounterRates::FifoReadResult::Failure:
            os << "FAILURE";
            break;
        case CounterRates::FifoReadResult::SuccessNoRates:
            os << "SUCCESS_NO_RATES";
            break;
        case CounterRates::FifoReadResult::SuccessCleared:
            os << "SUCCES_CLEARED";
            break;
        case CounterRates::FifoReadResult::Success:
            os << "SUCCESS";
            break;
        case CounterRates::FifoReadResult::NotPerformed:
            os << "NOT_PERFORMED";
            break;
    }
    return os;
}

string CounterRates::generateResponse(ReadIntervalState readIntervalState, FifoState fifoState, uint32_t fifoLoad, FifoReadResult fifoReadResult) const
{
    stringstream ss;
    ss << "READ_INTERVAL," << readIntervalState << ',' << m_readInterval << "s\nFIFO_STATE," << fifoState << ',' << fifoLoad << "\nFIFO_READ_RESULT," << fifoReadResult << "\nCOUNTERS";
    if (m_counters.has_value()) {
        for (auto c : *m_counters) {
            ss << "," << c;
        }
    } else {
        ss << ",-";
    }
    ss << "\nRATES";
    if (m_rates.has_value()) {
        for (auto r : *m_rates) {
            ss << "," << r;
        }
    } else {
        ss << ",-";
    }

    return ss.str();
}