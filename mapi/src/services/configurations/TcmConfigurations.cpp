#include "services/configurations/TcmConfigurations.h"
#include "utils/DelayChange.h"
#include "board/TCM.h"
#include <unistd.h>

TcmConfigurations::TcmConfigurations(std::shared_ptr<Board> board) : BoardConfigurations(board)
{
    if (!board->doesExist(string(tcm_parameters::DelayA)) || !board->doesExist(string(tcm_parameters::DelayC))) {
        throw runtime_error("Couldn't construct TcmConfigurations: no delay parameters");
    }
}

bool TcmConfigurations::handleDelays()
{
    optional<DelayChange> delayChange = DelayChange::fromElectronicValues(m_handler, m_configurationInfo.delayA, m_configurationInfo.delayC);

    if (!delayChange.has_value()) {
        return true;
    }

    Print::PrintVerbose("Delay difference " + to_string(delayChange->delayDifference) + ", req:\n" + delayChange->req);

    auto parsedResponse = delayChange->apply(*this, m_handler, false); // Readiness changed bits will be cleared afterwards
    m_response += parsedResponse.getContents();
    if (parsedResponse.isError()) {
        m_response.insert(0, "TCM configuration " + m_configurationInfo.name + " was not applied: delay change failed\n");
        printAndPublishError(m_response);
        return false;
    }

    return true;
}

bool TcmConfigurations::handleData()
{
    Print::PrintVerbose("Applying data, req:\n" + m_configurationInfo.req);
    auto parsedResponse = processSequenceThroughHandler(m_handler, m_configurationInfo.req);
    m_response += parsedResponse.getContents();
    if (parsedResponse.isError()) {
        m_response.insert(0, "TCM configuration " + m_configurationInfo.name + (m_response.empty() ? " was not applied\n" : " was applied partially\n"));
        printAndPublishError(m_response);
        return false;
    }

    // Control Server sleeps 10 ms if CH_MASK_A or CH_MASK_C was changed
    // This waits for PM initialization (done automatically by TCM) to finish
    usleep(10000);
    return true;
}

void TcmConfigurations::handleResetErrors()
{
    // Control Server performs entire reset errors - shouldn't be needed
    string resetReq;
    WinCCRequest::appendToRequest(resetReq, WinCCRequest::writeRequest(tcm_parameters::SystemRestarted, 1));
    processSequenceThroughHandler(m_handler, resetReq, false);
    return;
}

void TcmConfigurations::processExecution()
{
    m_response.clear();

    bool running = true;
    string request = waitForRequest(running);
    if (running == false) {
        return;
    }

    const string& configurationName = request;
    m_configurationInfo = getConfigurationInfo(configurationName);
    Print::PrintVerbose("Configuration '" + configurationName + "' for " + m_boardName);

    if (!handleDelays()) {
        return;
    }
    if (!handleData()) {
        return;
    }

    // Required after change of delays and SIDE_[A/C]_CHANNEL_MASK
    // Performed always for simplicity
    // Clearing readiness changed bits should be enough - tests will show
    handleResetErrors();
    Print::PrintInfo("Configuration '" + m_configurationInfo.name + "' successfully applied to " + m_boardName);
    publishAnswer(m_response);
}
