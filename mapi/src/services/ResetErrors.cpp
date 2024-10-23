#include"services/ResetErrors.h"
#include"gbtInterfaceUtils.h"

ResetErrors::ResetErrors(std::shared_ptr<Board> tcm, std::vector<std::shared_ptr<Board>> pms): BasicFitIndefiniteMapi(tcm)
{
    appendRequest(m_reqClearResetBits,  writeRequest(gbt::parameters::Reset, 0));
    appendRequest(m_reqClearResetBits,  writeRequest(gbt::parameters::ResetDataCounters, 0));
    appendRequest(m_reqClearResetBits,  writeRequest(gbt::parameters::ResetStartEmulation, 0));
    appendRequest(m_reqClearResetBits,  writeRequest(gbt::parameters::ResetOrbitSync, 0));
    appendRequest(m_reqClearResetBits,  writeRequest(gbt::parameters::ResetReadoutFsm, 0));
    appendRequest(m_reqClearResetBits,  writeRequest(gbt::parameters::ResetRxError, 0));
    appendRequest(m_reqClearResetBits,  writeRequest(gbt::parameters::ResetRxPhaseError, 0));
    appendRequest(m_reqClearResetBits,  writeRequest(gbt::parameters::FifoReportReset, 0));

    appendRequest(m_reqClearAndUnlock,  writeRequest(gbt::parameters::Reset, 0));
    appendRequest(m_reqClearAndUnlock,  writeRequest(gbt::parameters::ResetDataCounters, 0));
    appendRequest(m_reqClearAndUnlock,  writeRequest(gbt::parameters::ResetStartEmulation, 0));
    appendRequest(m_reqClearAndUnlock,  writeRequest(gbt::parameters::ResetOrbitSync, 0));
    appendRequest(m_reqClearAndUnlock,  writeRequest(gbt::parameters::ResetReadoutFsm, 0));
    appendRequest(m_reqClearAndUnlock,  writeRequest(gbt::parameters::ResetRxError, 0));
    appendRequest(m_reqClearAndUnlock,  writeRequest(gbt::parameters::ResetRxPhaseError, 0));
    appendRequest(m_reqClearAndUnlock,  writeRequest(gbt::parameters::FifoReportReset, 0));
    appendRequest(m_reqClearAndUnlock,  writeRequest(gbt::parameters::ForceIdle, 0));

    appendRequest(m_reqApplyResets, writeRequest(gbt::parameters::ResetRxError, 1));
    appendRequest(m_reqApplyResets, writeRequest(gbt::parameters::ResetReadoutFsm, 1));
    appendRequest(m_reqApplyResets, writeRequest(gbt::parameters::FifoReportReset, 1));

    for (auto& pm : pms) {
            m_PMs.emplace_back(pm);
    }
}

void ResetErrors::processExecution()
{
    bool running = true;
    bool success = false;

    std::string request = waitForRequest(running);
    if (running == false) {
        return;
    }

    {
        auto parsedResponse = applyResetBoard(*this);
        if(parsedResponse.isError()){
            publishError(parsedResponse.getContents());
            return;
        }
    }

    for(auto& pmHandler: m_PMs)
    {
        auto parsedResponse = applyResetBoard(pmHandler);
        if(parsedResponse.isError()){
            publishError(parsedResponse.getContents());
            return;
        }
    }

    publishAnswer("SUCCESS");
}

BasicRequestHandler::ParsedResponse ResetErrors::applyResetBoard(BasicRequestHandler& boardHandler)
{
    {
        auto parsedResponse = processSequence(boardHandler, m_reqClearResetBits);
        if(parsedResponse.isError()){
            return parsedResponse;
        }
    }

    {
        auto parsedResponse = processSequence(boardHandler, m_reqApplyResets);
        if(parsedResponse.isError()){
            return parsedResponse;
        }   
    }

    {
        auto parsedResponse = processSequence(boardHandler, m_reqClearAndUnlock);
        if(parsedResponse.isError()){
            return parsedResponse;
        }
    }

    return EmptyResponse;
}