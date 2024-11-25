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
        if (!currReadIntervalCode.has_value()) {
            // Wait until TCM reads the value
            usleep(100'000);
            currReadIntervalCode = board->getParentBoard()->at("COUNTER_READ_INTERVAL").getElectronicValueOptional();
        }
    }

    if (!currReadIntervalCode.has_value() || *currReadIntervalCode < 0 || *currReadIntervalCode > 7)
        return ReadIntervalState::Invalid;

    double currReadInterval = mapReadIntervalCodeToSeconds(*currReadIntervalCode);
    bool readIntervalChanged = (currReadInterval != m_readInterval);
    m_readInterval = currReadInterval;

    if (currReadInterval == 0) {
        return ReadIntervalState::Disabled;
    } else if (readIntervalChanged) {
        resetService();
        return ReadIntervalState::Changed;
    } else {
        return ReadIntervalState::Ok;
    }
}

CounterRates::FifoState CounterRates::evaluateFifoState(uint32_t fifoLoad) const
{
    if (fifoLoad == 0xFFFFFFFF) {
        return FifoState::BoardError;
    } else if (fifoLoad == 0) {
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

CounterRates::FifoReadResult CounterRates::handleCounterValues(const BoardCommunicationHandler::FifoResponse&& fifoResult, bool clearOnly)
{
    if (fifoResult.isError())
        return FifoReadResult::Failure;

    if (clearOnly) {
        resetService();
        return FifoReadResult::SuccessCleared;
    }

    const vector<vector<uint32_t>>& counterValues = fifoResult.fifoContent;

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

optional<CounterRates::ReadoutResult> CounterRates::handleDirectReadout()
{
    publishError("Direct counter readout mode unsupported");
    usleep(1'000'000);
    return nullopt;
}

optional<CounterRates::ReadoutResult> CounterRates::handleFifoReadout(ReadIntervalState readIntervalState)
{
    optional<uint32_t> fifoLoad = getFifoLoad();
    if (!fifoLoad.has_value()) {
        publishError("Couldn't read FIFO state");
        return nullopt;
    }

    FifoState fifoState;
    if (readIntervalState == ReadIntervalState::Changed) {
        fifoState = FifoState::Outdated;
    } else {
        fifoState = evaluateFifoState(*fifoLoad);
    }
    if (fifoState == FifoState::BoardError) {
        publishError("A board error occurred on FIFO_LOAD readout");
        return nullopt;
    } else if (fifoState == FifoState::Unexpected) {
        publishError("Unexpected FIFO_LOAD state - clearing fifo");
    }

    FifoReadResult fifoReadResult = FifoReadResult::NotPerformed;
    if (fifoState == FifoState::Single || fifoState == FifoState::Multiple) {
        fifoReadResult = readFifo(*fifoLoad);
    } else if (fifoState == FifoState::Outdated || fifoState == FifoState::Unexpected) {
        fifoReadResult = clearFifo(*fifoLoad);
    }

    if (fifoReadResult == FifoReadResult::Failure) {
        publishError("Couldn't read FIFO");
        return nullopt;
    }

    return ReadoutResult(readIntervalState, m_readInterval, fifoState, *fifoLoad, fifoReadResult, m_counters, m_rates);
}

vector<uint32_t> CounterRates::readDirectly()
{
    string request;
    for (const auto& name : m_names)
        WinCCRequest::appendToRequest(request, WinCCRequest::readRequest(name));
    auto parsedResponse = processSequenceThroughHandler(m_handler, request);
    if (parsedResponse.isError())
        return {};

    vector<uint32_t> result;
    result.reserve(m_numberOfCounters);
    for (const auto& name : m_names)
        result.push_back(m_handler.getBoard()->at(name).getElectronicValue());
    return result;
}

bool CounterRates::resetCounters()
{
    string flagName = m_handler.getBoard()->isTcm() ? "BOARD_STATUS_RESET_COUNTERS" : "RESET_COUNTERS_AND_HISTOGRAMS";
    // additional read from FIFO (like in CS) shouldn't be required:
    // reset request is handled directly after last read from FIFO

    vector<uint32_t> directCounters = readDirectly();
    if (directCounters.empty())
        return false;

    auto parsedResponse = processSequenceThroughHandler(m_handler, WinCCRequest::writeRequest(flagName, 1), false);
    if (parsedResponse.isError())
        return false;

    if (m_counters.has_value())
        for (size_t i = 0; i < m_counters->size(); i++)
            // underflow generated on purpose - works like a negative number for subtraction in rates calculation
            // required to compensate for the fact that zeroing of counters can occur anywhere between FIFO loads
            m_counters->at(i) -= directCounters[i];
    // m_counters->at(i) = 0;

    return true;
}

void CounterRates::pollResetCounters() {
    bool running;
    if (!isRequestAvailable(running))
        return;
    
    if(!running)
        return;
    
    bool isResetPending = false;
    while (isRequestAvailable(running)) {
        if(!running)
            return;
        string request = getRequest();
        if (request == "RESET") 
            isResetPending = true;
        else
            Print::PrintWarning("Unexpected request: " + request);
    }

    if (isResetPending)
        resetCounters() ? Print::PrintInfo("Succesfully reset counters") : throw runtime_error("Counter reset failed");
}

void CounterRates::processExecution()
{
    bool running;
    usleep(getSleepDuration());

    startTimeMeasurement();

    // Fixes segfault after SIGINT termination
    isRequestAvailable(running);
    if (!running)
        return;

#ifdef FIT_UNIT_TEST
    ReadIntervalState readIntervalState = ReadIntervalState::Ok;
#else
    ReadIntervalState readIntervalState = handleReadInterval();
#endif

    optional<ReadoutResult> readoutResult;
    if (readIntervalState == ReadIntervalState::Invalid) {
        publishError("Invalid read interval");
        return;
    } else if (readIntervalState == ReadIntervalState::Disabled) {
        readoutResult = handleDirectReadout();
    } else {
        readoutResult = handleFifoReadout(readIntervalState);
    }

    // Allow reset only if counter FIFO has recently been cleared or readout is disabled
    // (minimises chance of incorrect rate calculation afterwards)
    if ((readIntervalState == ReadIntervalState::Disabled || (readoutResult.has_value() && readoutResult->fifoReadResult != FifoReadResult::NotPerformed))) {
        pollResetCounters();
    }

    stopTimeMeasurement();

    if (readoutResult.has_value()) {
        string response = readoutResult->getString();// + "\nElapsed: " + to_string(m_elapsed);
        Print::PrintVerbose(response);
        publishAnswer(response);
    }
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
        default:
            os << "_INTERNAL_ERROR";
    }

    return os;
}

ostream& operator<<(ostream& os, CounterRates::FifoState fifoState)
{
    switch (fifoState) {
        case CounterRates::FifoState::BoardError:
            os << "BOARD_ERROR";
            break;
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
        default:
            os << "_INTERNAL_ERROR";
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
        default:
            os << "_INTERNAL_ERROR";
    }
    return os;
}

string CounterRates::ReadoutResult::getString() const
{
    stringstream ss;
    ss
        << "READ_INTERVAL," << readIntervalState << ',' << readInterval
        << "s\nFIFO_STATE," << fifoState << ',' << fifoLoad
        << "\nFIFO_READ_RESULT," << fifoReadResult;

    ss << "\nCOUNTERS";
    if (counters.has_value()) {
        for (auto c : *counters) {
            ss << "," << c;
        }
    } else {
        ss << ",-";
    }

    ss << "\nRATES";
    if (rates.has_value()) {
        for (auto r : *rates) {
            ss << "," << r;
        }
    } else {
        ss << ",-";
    }

    return ss.str();
}