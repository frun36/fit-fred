#include "services/Configurations.h"
#include <cctype>
#include <unistd.h>
#include <sstream>
#include "utils.h"
#include <database/sql.h>
#include <database/DatabaseTables.h>
#include <Alfred/print.h>
#include "DelayChange.h"

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

bool Configurations::TcmConfigurations::handleDelays(const string& configurationName, const ConfigurationInfo& configurationInfo, string& response)
{
    optional<DelayChange> delayChange = DelayChange::fromValues(m_tcm, configurationInfo.delayA, configurationInfo.delayC);

    if (!delayChange.has_value()) {
        return true;
    }

    Print::PrintVerbose("Delay difference " + to_string(delayChange->delayDifference) + ", req:\n" + delayChange->req);

    auto parsedResponse = delayChange->apply(*this, m_tcm);
    response += parsedResponse.getContents();
    if (parsedResponse.isError()) {
        response.insert(0, "TCM configuration " + configurationName + " was not applied: delay change failed\n");
        publishError(response);
        return false;
    }

    return true;
}

bool Configurations::TcmConfigurations::handleData(const string& configurationName, const ConfigurationInfo& configurationInfo, string& response)
{
    Print::PrintVerbose("Applying data, req:\n" + configurationInfo.req);
    auto parsedResponse = processSequenceThroughHandler(m_tcm, configurationInfo.req);
    response += parsedResponse.getContents();
    if (parsedResponse.isError()) {
        response.insert(0, "TCM configuration " + configurationName + (response.empty() ? " was not applied\n" : " was applied partially\n"));
        publishError(response);
        return false;
    }

    // Control Server sleeps 10 ms if CH_MASK_A or CH_MASK_C was changed
    // This waits for PM initialization (done automatically by TCM) to finish
    usleep(10000);
    return true;
}

void Configurations::TcmConfigurations::handleResetErrors()
{
    // Control Server performs entire reset errors - shouldn't be needed
    string resetReq;
    WinCCRequest::appendToRequest(resetReq, WinCCRequest::writeRequest("BOARD_STATUS_SYSTEM_RESTARTED", 1));
    processSequenceThroughHandler(m_tcm, resetReq, false);
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
    Print::PrintVerbose("Configuration '" + configurationName + "' for " + string(getBoardName()));

    string response;
    if (!handleDelays(configurationName, configurationInfo, response))
        return;
    if (!handleData(configurationName, configurationInfo, response))
        return;
    handleResetErrors(); // Not sure if it's needed after CH_MASK_A/C - done already in DelayChange
    publishAnswer(response);
}
