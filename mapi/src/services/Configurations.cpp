#include "services/Configurations.h"
#include <cctype>
#include <unistd.h>
#include <sstream>
#include "utils.h"

Configurations::Configurations(const string& fredName, const unordered_map<string, shared_ptr<Board>>& boards) : m_fredName(fredName)
{
    for (auto [name, board] : boards) {
        if (name == "TCM")
            m_boardCofigurationServices[name] = make_unique<TcmConfigurations>(board);
        else
            m_boardCofigurationServices[name] = make_unique<PmConfigurations>(board);
    }
}

string Configurations::processInputMessage(string msg)
{
    if (!DatabaseInterface::isConnected())
        throw runtime_error("No DB connection");

    Utility::removeWhiteSpaces(msg);
    const string& configurationName = msg;

    auto boardNamesData = DatabaseInterface::executeQuery("SELECT DISTINCT board_name FROM configuration_parameters WHERE configuration_name = '" + configurationName + "'");
    if (boardNamesData.empty())
        throw runtime_error(configurationName + ": configuration not found");

    vector<pair<string, string>> requests(boardNamesData.size());
    std::transform(boardNamesData.begin(), boardNamesData.end(), requests.begin(), [&configurationName, this](const vector<MultiBase*>& entry) {
        if (!entry[0]->isString())
            throw runtime_error(configurationName + ": invalid board name format in DB");

        string boardName = entry[0]->getString();
        string serviceName = m_fredName;
        if (boardName == "TCM0")
            serviceName += "/TCM/TCM0/";
        else if (boardName.find("PM") != string::npos)
            serviceName += "PM/" + boardName + "/";
        serviceName += "_INTERNAL_CONFIGURATIONS";

        return make_pair(serviceName, configurationName);
    });

    newMapiGroupRequest(requests);
    noRpcRequest = true;
    return "";
}

std::vector<std::vector<MultiBase*>> Configurations::BoardConfigurations::fetchConfiguration(string_view configuration, string_view board)
{
    return DatabaseInterface::executeQuery("SELECT parameter_name, parameter_value FROM configuration_parameters WHERE configuration_name = '" + string(configuration) + "' AND board_name = '" + string(board) + "'");
}

Configurations::BoardConfigurations::ConfigurationInfo Configurations::BoardConfigurations::getConfigurationInfo(string_view configurationName, const vector<vector<MultiBase*>>& dbData)
{
    optional<double> delayA = nullopt;
    optional<double> delayC = nullopt;
    string request;
    for (const auto& row : dbData) {
        if (row.size() != 2 || !row[0]->isString() || !row[1]->isDouble())
            throw runtime_error(string_utils::concatenate(configurationName, ": invalid CONFIGURATIONS data row"));

        string parameterName = row[0]->getString();
        double parameterValue = row[1]->getDouble();
        if (parameterName == "DELAY_A")
            delayA = parameterValue;
        else if (parameterName == "DELAY_C")
            delayC = parameterValue;
        else
            WinCCRequest::appendToRequest(request, WinCCRequest::writeRequest(parameterName, parameterValue));
    }

    return ConfigurationInfo(request, delayA, delayC);
}

string Configurations::PmConfigurations::processInputMessage(string name)
{
    return m_pm.processMessageFromWinCC(fetchAndGetConfigurationInfo(name).req).getSequence();
}

string Configurations::PmConfigurations::processOutputMessage(string msg)
{
    auto parsedResponse = m_pm.processMessageFromALF(msg);
    if (parsedResponse.isError())
        returnError = true;
    return parsedResponse.getContents();
}

optional<SwtSequence> Configurations::TcmConfigurations::processDelayInput(optional<double> delayA, optional<double> delayC)
{
    if (!delayA.has_value() && !delayC.has_value())
        return nullopt;

    m_delayDifference = 0;

    string request;

    if (delayA.has_value()) {
        m_delayDifference = abs(delayA.value() - getDelayA().value_or(0));
        if (m_delayDifference != 0)
            WinCCRequest::appendToRequest(request, WinCCRequest::writeRequest("DELAY_A", delayA.value()));
    }

    if (delayC.has_value()) {
        int16_t cDelayDifference = abs(delayC.value() - getDelayC().value_or(0));
        if (cDelayDifference > m_delayDifference)
            m_delayDifference = cDelayDifference;
        if (cDelayDifference != 0)
            WinCCRequest::appendToRequest(request, WinCCRequest::writeRequest("DELAY_C", delayC.value()));
    }

    return m_tcm.processMessageFromWinCC(request);
}

bool Configurations::TcmConfigurations::handleDelays(string& response)
{
    optional<SwtSequence> delaySequence = processDelayInput(m_configurationInfo->delayA, m_configurationInfo->delayC);

    if (!delaySequence.has_value()) {
        return true;
    }

    string delayResponse = executeAlfSequence(delaySequence->getSequence());
    auto parsedResponse = m_tcm.processMessageFromALF(delayResponse);
    string parsedResponseString = parsedResponse.getContents();
    response += parsedResponseString;
    if (parsedResponse.isError()) {
        response.insert(0, "TCM configuration " + m_configurationName.value_or("<no name>") + " was not applied: delay change failed\n");
        reset();
        publishError(response);
        return false;
    }

    // Change of delays/phase is slowed down to 1 unit/ms in the TCM logic, to avoid PLL relock.
    // For larger changes however, the relock will occur nonetheless, causing the BOARD_STATUS_SYSTEM_RESTARTED bit to be set.
    // This sleep waits for the phase shift to finish, and the bit will be cleared later in the procedure
    usleep((static_cast<useconds_t>(m_tcm.getBoard()->calculateElectronic("DELAY_A", m_delayDifference)) + 10) * 1000);
    return true;
}

bool Configurations::TcmConfigurations::handleData(string& response)
{
    SwtSequence dataSequence = m_tcm.processMessageFromWinCC(m_configurationInfo->req);

    string dataResponse = executeAlfSequence(dataSequence.getSequence());
    auto parsedResponse = m_tcm.processMessageFromALF(dataResponse);
    string parsedResponseString = parsedResponse.getContents();
    response += parsedResponseString;
    if (parsedResponse.isError()) {
        response.insert(0, "TCM configuration " + m_configurationName.value_or("<no name>") + (response.empty() ? " was not applied\n" : " was applied partially\n"));
        publishError(response);
        return false;
    }

    // Control Server sleeps 10 ms if CH_MASK_A or CH_MASK_C was changed
    usleep(10000);
    return true;
}

void Configurations::TcmConfigurations::handleResetErrors()
{
    // Control Server performs entire reset errors - to be tested
    string resetReq;
    WinCCRequest::appendToRequest(resetReq, WinCCRequest::writeRequest("BOARD_STATUS_RESET_ERRORS", 1));
    SwtSequence resetSequence = m_tcm.processMessageFromWinCC(resetReq, false);
    executeAlfSequence(resetSequence.getSequence());
    return;
}

void Configurations::TcmConfigurations::processExecution()
{
    bool running = true;
    string request = waitForRequest(running);
    if (running == false) {
        return;
    }

    m_configurationName.emplace(request);
    m_configurationInfo.emplace(fetchAndGetConfigurationInfo(*m_configurationName));

    string response;
    if (!handleDelays(response))
        return;
    if (!handleData(response))
        return;
    handleResetErrors();
    reset();
    publishAnswer(response);
}