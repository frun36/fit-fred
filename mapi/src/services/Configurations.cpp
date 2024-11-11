#include "services/Configurations.h"
#include <cctype>
#include <unistd.h>
#include <sstream>
#include "utils.h"
#include <Database/sql.h>
#include <Database/DatabaseTables.h>
#include <Alfred/print.h>

Configurations::Configurations(const string& fredName, const unordered_map<string, shared_ptr<Board>>& boards) : m_fredName(fredName)
{
    string names;
    for (auto [name, board] : boards) {
        if (name.find("TCM") != string::npos)
            m_boardCofigurationServices[name] = make_unique<TcmConfigurations>(board);
        else if (name.find("PM") != string::npos)
            m_boardCofigurationServices[name] = make_unique<PmConfigurations>(board);
        else
            throw runtime_error("Unexpected board name: " + name);
        names += name + "; ";
    }
    Print::PrintInfo("CONFIGURATIONS initialized for boards: " + names);
}

string Configurations::processInputMessage(string msg)
{
    if (!DatabaseInterface::isConnected())
        throw runtime_error("No DB connection");

    Utility::removeWhiteSpaces(msg);
    const string& configurationName = msg;

    sql::SelectModel query;
    query
        .select(db_tables::ConfigurationParameters::BoardName.name)
        .distinct()
        .from(db_tables::ConfigurationParameters::TableName)
        .where(sql::column(db_tables::ConfigurationParameters::ConfigurationName.name) == configurationName);
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
            serviceName += "/PM/" + boardName + "/";
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
        .select(db_tables::ConfigurationParameters::ParameterName.name, db_tables::ConfigurationParameters::Value.name)
        .from(db_tables::ConfigurationParameters::TableName)
        .where(sql::column(db_tables::ConfigurationParameters::ConfigurationName.name) == string(configurationName) && sql::column(db_tables::ConfigurationParameters::BoardName.name) == string(boardName));
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
    const string& req = fetchAndGetConfigurationInfo(name).req;
    Print::PrintVerbose("Configuration '" + name + "' for " + string(getBoardName()) + ":\n" + req);
    return m_pm.processMessageFromWinCC(req).getSequence();
}

string Configurations::PmConfigurations::processOutputMessage(string msg)
{
    auto parsedResponse = m_pm.processMessageFromALF(msg);
    if (parsedResponse.isError())
        returnError = true;
    return parsedResponse.getContents();
}

// TcmConfigurations

optional<Configurations::TcmConfigurations::DelayInfo> Configurations::TcmConfigurations::processDelayInput(optional<double> newDelayA, optional<double> newDelayC)
{
    if (!newDelayA.has_value() && !newDelayC.has_value())
        return nullopt;

    string request;
    uint32_t delayDifference = 0;

    if (newDelayA.has_value()) {
        int64_t newDelayAElectronic = m_tcm.getBoard()->calculateElectronic("DELAY_A", *newDelayA);
        delayDifference = abs(newDelayAElectronic - getDelayAElectronic().value_or(0));
        if (delayDifference != 0)
            WinCCRequest::appendToRequest(request, WinCCRequest::writeRequest("DELAY_A", newDelayA.value()));
    }

    if (newDelayC.has_value()) {
        int64_t newDelayCElectronic = m_tcm.getBoard()->calculateElectronic("DELAY_C", *newDelayC);
        uint32_t cDelayDifference = abs(newDelayCElectronic - getDelayCElectronic().value_or(0));
        if (cDelayDifference > delayDifference)
            delayDifference = cDelayDifference;
        if (cDelayDifference != 0)
            WinCCRequest::appendToRequest(request, WinCCRequest::writeRequest("DELAY_C", newDelayC.value()));
    }

    return make_optional<DelayInfo>(request, delayDifference);
}

bool Configurations::TcmConfigurations::handleDelays(const string& configurationName, const ConfigurationInfo& configurationInfo, string& response)
{
    optional<DelayInfo> delayInfo = processDelayInput(configurationInfo.delayA, configurationInfo.delayC);

    if (!delayInfo.has_value()) {
        return true;
    }

    Print::PrintVerbose("Delay difference " + to_string(delayInfo->delayDifference) + ", req:\n" + delayInfo->req);

    auto parsedResponse = processSequenceThroughHandler(m_tcm, delayInfo->req);
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
    Print::PrintVerbose("Applying data, req:\n" + configurationInfo.req);
    auto parsedResponse = processSequenceThroughHandler(m_tcm, configurationInfo.req);
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
    WinCCRequest::appendToRequest(resetReq, WinCCRequest::writeRequest("BOARD_STATUS_SYSTEM_RESTARTED", 1));
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

    const string& configurationName = request;
    ConfigurationInfo configurationInfo = fetchAndGetConfigurationInfo(configurationName);
    Print::PrintVerbose("Configuration '" + name + "' for " + string(getBoardName()));

    string response;
    if (!handleDelays(configurationName, configurationInfo, response))
        return;
    if (!handleData(configurationName, configurationInfo, response))
        return;
    handleResetErrors();
    publishAnswer(response);
}
