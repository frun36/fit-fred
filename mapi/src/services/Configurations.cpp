#include "services/Configurations.h"
#include <cctype>
#include <unistd.h>
#include <sstream>
#include "utils.h"
#include "database/sql.h"
#include "database/DatabaseTables.h"
#include "Alfred/print.h"
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

        return make_pair(m_boardCofigurationServices[boardName]->getServiceName(), configurationName);
    });

    newMapiGroupRequest(requests);
    noRpcRequest = true;
    return "";
}
