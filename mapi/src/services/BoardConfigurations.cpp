#include "services/BoardConfigurations.h"
#include "database/sql.h"
#include "database/BoardConfigurationQueries.h"
#include "DelayChange.h"

// BoardConfigurations

std::vector<std::vector<MultiBase*>> BoardConfigurations::fetchConfiguration(const std::string& configurationName, const std::string& boardName)
{
    return DatabaseInterface::executeQuery(db_fit::queries::selectBoardConfiguration(configurationName, boardName));
}

BoardConfigurations::ConfigurationInfo BoardConfigurations::parseConfigurationInfo(const std::string& configurationName, const vector<vector<MultiBase*>>& dbData)
{
    optional<int64_t> delayA = nullopt;
    optional<int64_t> delayC = nullopt;
    string request;
    for (const auto& row : dbData) {
        db_fit::tables::ConfigurationParameters::Row parsedRow(row);
        if (parsedRow.parameterName == tcm_parameters::DelayA) {
            delayA = parsedRow.parameterValue;
        } else if (parsedRow.parameterName == tcm_parameters::DelayC) {
            delayC = parsedRow.parameterValue;
        } else {
            WinCCRequest::appendToRequest(request, WinCCRequest::writeElectronicRequest(parsedRow.parameterName, parsedRow.parameterValue));
        }
    }

    return ConfigurationInfo(string(configurationName), request, delayA, delayC);
}

// PmConfigurations

string PmConfigurations::processInputMessage(string request)
{
    const string& configurationName = request;
    m_configurationInfo = getConfigurationInfo(configurationName);
    const string& req = m_configurationInfo.req;
    Print::PrintVerbose("Configuration '" + name + "' for " + m_boardName + ":\n" + req);
    return m_handler.processMessageFromWinCC(req).getSequence();
}

string PmConfigurations::processOutputMessage(string msg)
{
    auto parsedResponse = m_handler.processMessageFromALF(msg);
    if (parsedResponse.isError()) {
        returnError = true;
    } else {
        Print::PrintInfo("Configuration '" + m_configurationInfo.name + "' successfully applied to " + m_boardName);
    }
    return parsedResponse.getContents();
}

// TcmConfigurations

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
