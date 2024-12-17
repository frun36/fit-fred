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
        if (name.find("TCM") != string::npos) {
            m_boardCofigurationServices[name] = make_unique<TcmConfigurations>(board);
        } else if (name.find("PM") != string::npos) {
            m_boardCofigurationServices[name] = make_unique<PmConfigurations>(board);
        } else {
            throw runtime_error("Unexpected board name: " + name);
        }
        names += name + "; ";
    }
    Print::PrintInfo("CONFIGURATIONS initialized for boards: " + names);
}

vector<string> Configurations::fetchBoardNamesToConfigure(const string& configurationName) const
{
    sql::SelectModel query;
    query
        .select(db_tables::ConfigurationParameters::BoardName.name)
        .distinct()
        .from(db_tables::ConfigurationParameters::TableName)
        .where(sql::column(db_tables::ConfigurationParameters::ConfigurationName.name) == configurationName);
    auto boardNameData = DatabaseInterface::executeQuery(query.str());

    vector<string> names(boardNameData.size());
    std::transform(boardNameData.begin(), boardNameData.end(), names.begin(), [&configurationName, this](const vector<MultiBase*>& entry) {
        if (entry.empty() || !entry[0]->isString()) {
            throw runtime_error(configurationName + ": invalid board name format in DB");
        }
        return entry[0]->getString();
    });

    return names;
}

string Configurations::processInputMessage(string msg)
{
    if (!DatabaseInterface::isConnected()) {
        throw runtime_error("No DB connection");
    }

    Utility::removeWhiteSpaces(msg);
    const string& configurationName = msg;

    vector<string> boardNames = fetchBoardNamesToConfigure(configurationName);
    if (boardNames.empty()) {
        throw runtime_error(configurationName + ": configuration not found");
    }

    vector<pair<string, string>> requests(boardNames.size());
    std::transform(boardNames.begin(), boardNames.end(), requests.begin(), [&configurationName, this](const auto& boardName) {
        if (m_boardCofigurationServices.find(boardName) == m_boardCofigurationServices.end()) {
            throw runtime_error(configurationName + ": board '" + boardName + "' is not connected");
        }

        return make_pair(m_boardCofigurationServices[name]->getServiceName(), configurationName);
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
        .select(db_tables::ConfigurationParameters::ParameterName.name, db_tables::ConfigurationParameters::ParameterValue.name)
        .from(db_tables::ConfigurationParameters::TableName)
        .where(sql::column(db_tables::ConfigurationParameters::ConfigurationName.name) == string(configurationName) && sql::column(db_tables::ConfigurationParameters::BoardName.name) == string(boardName));
    return DatabaseInterface::executeQuery(query.str());
}

Configurations::BoardConfigurations::ConfigurationInfo Configurations::BoardConfigurations::parseConfigurationInfo(string_view configurationName, const vector<vector<MultiBase*>>& dbData)
{
    optional<double> delayA = nullopt;
    optional<double> delayC = nullopt;
    string request;
    for (const auto& row : dbData) {
        if (row.size() != 2 || !row[0]->isString() || !row[1]->isDouble()) {
            throw runtime_error(string_utils::concatenate(configurationName, ": invalid CONFIGURATIONS data row"));
        }

        string parameterName = row[0]->getString();
        double parameterValue = row[1]->getDouble();
        if (parameterName == tcm_parameters::DelayA) {
            delayA = parameterValue;
        } else if (parameterName == tcm_parameters::DelayC) {
            delayC = parameterValue;
        } else {
            WinCCRequest::appendToRequest(request, WinCCRequest::writeRequest(parameterName, parameterValue));
        }
    }

    return ConfigurationInfo(string(configurationName), request, delayA, delayC);
}

// PmConfigurations

string Configurations::PmConfigurations::processInputMessage(string request)
{
    const string& configurationName = request;
    m_configurationInfo = getConfigurationInfo(configurationName);
    const string& req = m_configurationInfo.req;
    Print::PrintVerbose("Configuration '" + name + "' for " + m_boardName + ":\n" + req);
    return m_handler.processMessageFromWinCC(req).getSequence();
}

string Configurations::PmConfigurations::processOutputMessage(string msg)
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

bool Configurations::TcmConfigurations::handleDelays()
{
    optional<DelayChange> delayChange = DelayChange::fromValues(m_handler, m_configurationInfo.delayA, m_configurationInfo.delayC);

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

bool Configurations::TcmConfigurations::handleData()
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

void Configurations::TcmConfigurations::handleResetErrors()
{
    // Control Server performs entire reset errors - shouldn't be needed
    string resetReq;
    WinCCRequest::appendToRequest(resetReq, WinCCRequest::writeRequest("BOARD_STATUS_SYSTEM_RESTARTED", 1));
    processSequenceThroughHandler(m_handler, resetReq, false);
    return;
}

void Configurations::TcmConfigurations::processExecution()
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
