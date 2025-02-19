#pragma once

#include "BoardCommunicationHandler.h"
#include <optional>
#include "Database/databaseinterface.h"

class BoardConfigurations
{
   public:
    struct ConfigurationInfo {
        string name;
        string req;
        optional<double> delayA;
        optional<double> delayC;

        ConfigurationInfo() = default;
        ConfigurationInfo(const string& name, const string& req, optional<int64_t> delayA, optional<int64_t> delayC);
    };

    static vector<vector<MultiBase*>> fetchConfiguration(const std::string& configurationName, const std::string& boardName);
    static ConfigurationInfo parseConfigurationInfo(const std::string& configurationName, const vector<vector<MultiBase*>>& dbData);

    ConfigurationInfo getConfigurationInfo(const std::string& configurationName) const;

    // Due to lack of virtual inheritance for IndefiniteMapi, I don't think we can have BoardConfigurations inherit Mapi
    // and implement this method here
    virtual const string& getServiceName() const = 0;

    virtual ~BoardConfigurations() = default;

   protected:
    BoardCommunicationHandler m_handler;
    ConfigurationInfo m_configurationInfo;
    const string& m_boardName;

    BoardConfigurations(std::shared_ptr<Board> board) : m_handler(board), m_boardName(board->getName()) {}
};
