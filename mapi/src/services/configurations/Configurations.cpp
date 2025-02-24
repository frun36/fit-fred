#include "services/configurations/Configurations.h"
#include "services/configurations/TcmConfigurations.h"
#include "services/configurations/PmConfigurations.h"
#include <cctype>
#include <unistd.h>
#include "database/ConfigurationsQueries.h"
#include "Alfred/print.h"

Configurations::Configurations(const string& fredName, const unordered_map<string, shared_ptr<Board>>& boards) : m_fredName(fredName)
{
    string names;
    for (auto [name, board] : boards) {
        if (!board->isConnected()) {
            continue;
        }

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
    auto boardNameData = DatabaseInterface::executeQuery(db_fit::queries::selectDistinctBoards(configurationName));

    vector<string> names(boardNameData.size());
    std::transform(boardNameData.begin(), boardNameData.end(), names.begin(), [&configurationName](const vector<MultiBase*>& entry) {
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

std::string Configurations::processOutputMessage(string msg)
{
    throw std::runtime_error("Configurations: unexpectedly received '" + msg + "' from ALF");
}
