#include"services/SetPhaseDelay.h"
#include"TCM.h"

void SetPhaseDelay::processExecution()
{
    bool running = true;

    std::string request = waitForRequest(running);
    if (running == false) {
        return;
    }

    bool sideA = (request.find("A") != std::string::npos);
    Board::ParameterInfo& delay = sideA ? m_handler.getBoard()->at(tcm_parameters::DelayA) : m_handler.getBoard()->at(tcm_parameters::DelayC);
    int64_t oldValue = (delay.getPhysicalValueOptional() != std::nullopt) ? static_cast<int64_t>(delay.getPhysicalValue()): 0;

    {
        auto response = processSequenceThroughHandler(m_handler, request);
        if (response.errors.empty() == false) {
            publishError(response.getContents());
            return;
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(abs(oldValue - static_cast<int64_t>(delay.getPhysicalValue()))));

    {
        auto response = processSequenceThroughHandler(m_handler,  WinCCRequest::writeRequest(tcm_parameters::SystemRestarted, 1), false);
        if (response.errors.empty() == false) {
            publishError(response.getContents());
            return;
        }
    }

    WinCCResponse response;
    response.addParameter(delay.name, {delay.getPhysicalValue()});
    publishAnswer(response.getContents());
}