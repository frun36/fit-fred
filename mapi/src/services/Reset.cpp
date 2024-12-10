#include "services/Reset.h"
#include "gbtInterfaceUtils.h"

Reset::Reset(std::shared_ptr<Board> board) : m_boardHandler(board)
{
    m_resetParameters.emplace(gbt::parameters::Reset, m_boardHandler.getBoard()->at(gbt::parameters::Reset));
    m_resetParameters.emplace(gbt::parameters::ResetDataCounters, m_boardHandler.getBoard()->at(gbt::parameters::ResetDataCounters));
    m_resetParameters.emplace(gbt::parameters::ResetStartEmulation, m_boardHandler.getBoard()->at(gbt::parameters::ResetStartEmulation));
    m_resetParameters.emplace(gbt::parameters::ResetOrbitSync, m_boardHandler.getBoard()->at(gbt::parameters::ResetOrbitSync));
    m_resetParameters.emplace(gbt::parameters::ResetReadoutFsm, m_boardHandler.getBoard()->at(gbt::parameters::ResetReadoutFsm));
    m_resetParameters.emplace(gbt::parameters::ResetRxError, m_boardHandler.getBoard()->at(gbt::parameters::ResetRxError));
    m_resetParameters.emplace(gbt::parameters::ResetRxPhaseError, m_boardHandler.getBoard()->at(gbt::parameters::ResetRxPhaseError));
    m_resetParameters.emplace(gbt::parameters::FifoReportReset, m_boardHandler.getBoard()->at(gbt::parameters::FifoReportReset));

    for (auto [name, parameter] : m_resetParameters) {
        WinCCRequest::appendToRequest(m_reqClearResetBits, WinCCRequest::writeRequest(name, 0));
    }
}

void Reset::processExecution()
{
    bool running = true;
    bool success = false;

    std::string request = waitForRequest(running);
    if (running == false) {
        return;
    }

    if (m_resetParameters.find(request) == m_resetParameters.end()) {
        publishError("Unexpected request: " + request);
    }

    {
        auto parsedResponse = processSequenceThroughHandler(m_boardHandler, m_reqClearResetBits);
        if (parsedResponse.isError()) {
            publishError(parsedResponse.getContents());
            return;
        }
    }

    {
        auto parsedResponse = processSequenceThroughHandler(m_boardHandler, WinCCRequest::writeRequest(request, 1));
        if (parsedResponse.isError()) {
            publishError(parsedResponse.getContents());
        } else {
            success = true;
        }
    }

    {
        auto parsedResponse = processSequenceThroughHandler(m_boardHandler, m_reqClearResetBits);
        if (parsedResponse.isError()) {
            publishError(parsedResponse.getContents());
        }
    }

    if (success) {
        publishAnswer("SUCCESS");
    }
}
