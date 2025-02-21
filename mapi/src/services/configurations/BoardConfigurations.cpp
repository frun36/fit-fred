#include "services/configurations/BoardConfigurations.h"
#include "database/BoardConfigurationQueries.h"
#include "database/DatabaseViews.h"
#include "board/TCM.h"

BoardConfigurations::ConfigurationInfo::ConfigurationInfo(const string& name, const string& req, optional<int64_t> delayA, optional<int64_t> delayC) : name(name), req(req), delayA(delayA), delayC(delayC) {}

BoardConfigurations::ConfigurationInfo BoardConfigurations::getConfigurationInfo(const std::string& configurationName) const
{
    auto dbData = fetchConfiguration(configurationName, m_boardName);
    return parseConfigurationInfo(configurationName, dbData);
}

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
        db_fit::views::ConfigurationValue::Row parsedRow(row);
        if (parsedRow.name == tcm_parameters::DelayA) {
            delayA = parsedRow.value;
        } else if (parsedRow.name == tcm_parameters::DelayC) {
            delayC = parsedRow.value;
        } else {
            WinCCRequest::appendToRequest(request, WinCCRequest::writeElectronicRequest(parsedRow.name, parsedRow.value));
        }
    }

    return ConfigurationInfo(string(configurationName), request, delayA, delayC);
}
