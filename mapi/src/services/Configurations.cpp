#include "services/Configurations.h"
#include <cctype>
#include <unistd.h>
#include <sstream>
#include "utils.h"
#include <Database/sql.h>

Configurations::Configurations(const string& fredName, const unordered_map<string, shared_ptr<Board>>& boards) : m_fredName(fredName)
{
    for (auto [name, board] : boards) {
        if (name.find("TCM") != string::npos)
            m_boardCofigurationServices[name] = make_unique<TcmConfigurations>(board);
        else if (name.find("PM") != string::npos)
            m_boardCofigurationServices[name] = make_unique<PmConfigurations>(board);
        else
            throw runtime_error("Unexpected board name: " + name);
    }
}

string Configurations::processInputMessage(string msg)
{
    if (!DatabaseInterface::isConnected())
        throw runtime_error("No DB connection");

    Utility::removeWhiteSpaces(msg);
    const string& configurationName = msg;

    sql::SelectModel query;
    query
        .select("board_name")
        .distinct()
        .from("configuration_parameters")
        .where(sql::column("configuration_name") == configurationName);
    auto boardNameData = DatabaseInterface::executeQuery(query.str());
    if (boardNameData.empty())
        throw runtime_error(configurationName + ": configuration not found");

    vector<pair<string, string>> requests(boardNameData.size());
    std::transform(boardNameData.begin(), boardNameData.end(), requests.begin(), [&configurationName, this](const vector<MultiBase*>& entry) {
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

// BoardConfigurations

std::vector<std::vector<MultiBase*>> Configurations::BoardConfigurations::fetchConfiguration(string_view configurationName, string_view boardName)
{
    sql::SelectModel query;
    query
        .select("parameter_name", "parameter_value")
        .from("configuration_parameters")
        .where(sql::column("configuration_name") == string(configurationName) && sql::column("board_name") == string(boardName));
    return DatabaseInterface::executeQuery(query.str());
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

// PmConfigurations

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

// TcmConfigurations

optional<int64_t> Configurations::TcmConfigurations::getDelayAElectronic() const
{
    optional<double> delayNs = m_tcm.getBoard()->at("DELAY_A").getStoredValueOptional();
    if (delayNs.has_value())
        return m_tcm.getBoard()->calculateElectronic("DELAY_A", *delayNs);
    else
        return nullopt;
}

optional<int64_t> Configurations::TcmConfigurations::getDelayCElectronic() const
{
    optional<double> delayNs = m_tcm.getBoard()->at("DELAY_C").getStoredValueOptional();
    if (delayNs.has_value())
        return m_tcm.getBoard()->calculateElectronic("DELAY_C", *delayNs);
    else
        return nullopt;
}

optional<Configurations::TcmConfigurations::DelayInfo> Configurations::TcmConfigurations::processDelayInput(optional<double> delayA, optional<double> delayC)
{
    if (!delayA.has_value() && !delayC.has_value())
        return nullopt;

    string request;
    uint32_t delayDifference;

    if (delayA.has_value()) {
        int64_t delayAElectronic = m_tcm.getBoard()->calculateElectronic("DELAY_A", *delayA);
        delayDifference = abs(delayAElectronic - getDelayAElectronic().value_or(0));
        if (delayDifference != 0)
            WinCCRequest::appendToRequest(request, WinCCRequest::writeRequest("DELAY_A", delayA.value()));
    }

    if (delayC.has_value()) {
        int64_t delayCElectronic = m_tcm.getBoard()->calculateElectronic("DELAY_C", *delayC);
        uint32_t cDelayDifference = abs(delayCElectronic - getDelayCElectronic().value_or(0));
        if (cDelayDifference > delayDifference)
            delayDifference = cDelayDifference;
        if (cDelayDifference != 0)
            WinCCRequest::appendToRequest(request, WinCCRequest::writeRequest("DELAY_C", delayC.value()));
    }

    return make_optional<DelayInfo>(request, delayDifference);
}

bool Configurations::TcmConfigurations::handleDelays(const string& configurationName, const ConfigurationInfo& configurationInfo, string& response)
{
    optional<DelayInfo> delayInfo = processDelayInput(configurationInfo.delayA, configurationInfo.delayC);

    if (!delayInfo.has_value()) {
        return true;
    }

    string delaySequence = m_tcm.processMessageFromWinCC(delayInfo->req).getSequence();
    string delayResponse = executeAlfSequence(delaySequence);
    auto parsedResponse = m_tcm.processMessageFromALF(delayResponse);
    string parsedResponseString = parsedResponse.getContents();
    response += parsedResponseString;
    if (parsedResponse.isError()) {
        response.insert(0, "TCM configuration " + configurationName + " was not applied: delay change failed\n");
        publishError(response);
        return false;
    }

    // Change of delays/phase is slowed down to 1 unit/ms in the TCM logic, to avoid PLL relock.
    // For larger changes however, the relock will occur nonetheless, causing the BOARD_STATUS_SYSTEM_RESTARTED bit to be set.
    // This sleep waits for the phase shift to finish, and the bit will be cleared later in the procedure
    usleep((static_cast<useconds_t>(delayInfo->delayDifference) + 10) * 1000);
    return true;
}

bool Configurations::TcmConfigurations::handleData(const string& configurationName, const ConfigurationInfo& configurationInfo, string& response)
{
    SwtSequence dataSequence = m_tcm.processMessageFromWinCC(configurationInfo.req);

    string dataResponse = executeAlfSequence(dataSequence.getSequence());
    auto parsedResponse = m_tcm.processMessageFromALF(dataResponse);
    string parsedResponseString = parsedResponse.getContents();
    response += parsedResponseString;
    if (parsedResponse.isError()) {
        response.insert(0, "TCM configuration " + configurationName + (response.empty() ? " was not applied\n" : " was applied partially\n"));
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

    string configurationName = request;
    ConfigurationInfo configurationInfo = fetchAndGetConfigurationInfo(configurationName);

    string response;
    if (!handleDelays(configurationName, configurationInfo, response))
        return;
    if (!handleData(configurationName, configurationInfo, response))
        return;
    handleResetErrors();
    publishAnswer(response);
}
