#include "services/Reset.h"
#include "gbtInterfaceUtils.h"

Reset::Reset(std::shared_ptr<Board> board) : BasicFitIndefiniteMapi(board)
{
    m_resetParameters.emplace(gbt::parameters::Reset, m_board->at(gbt::parameters::Reset));
    m_resetParameters.emplace(gbt::parameters::ResetDataCounters, m_board->at(gbt::parameters::ResetDataCounters));
    m_resetParameters.emplace(gbt::parameters::ResetDataCounters, m_board->at(gbt::parameters::ResetStartEmulation));
    m_resetParameters.emplace(gbt::parameters::ResetOrbitSync, m_board->at(gbt::parameters::ResetOrbitSync));
    m_resetParameters.emplace(gbt::parameters::ResetReadoutFsm, m_board->at(gbt::parameters::ResetReadoutFsm));
    m_resetParameters.emplace(gbt::parameters::ResetRxError, m_board->at(gbt::parameters::ResetRxError));
    m_resetParameters.emplace(gbt::parameters::ResetRxPhaseError, m_board->at(gbt::parameters::ResetRxPhaseError));
    m_resetParameters.emplace(gbt::parameters::FifoReportReset, m_board->at(gbt::parameters::FifoReportReset));

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
        auto parsedResponse = processSequence(m_reqClearResetBits);
        if (parsedResponse.isError()) {
            publishError(parsedResponse.getContents());
            return;
        }
    }

    {
        auto parsedResponse = processSequence(WinCCRequest::writeRequest(request, 1));
        if (parsedResponse.isError()) {
            publishError(parsedResponse.getContents());
        } else {
            success = true;
        }
    }

    {
        auto parsedResponse = processSequence(m_reqClearResetBits);
        if (parsedResponse.isError()) {
            publishError(parsedResponse.getContents());
        }
    }

    if (success) {
        publishAnswer("SUCCESS");
    }
}
