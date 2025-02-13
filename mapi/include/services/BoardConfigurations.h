#pragma once

#include "BoardCommunicationHandler.h"
#include <optional>
#include "services/BasicFitIndefiniteMapi.h"
#include "TCM.h"
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
        ConfigurationInfo(const string& name, const string& req, optional<int64_t> delayA, optional<int64_t> delayC) : name(name), req(req), delayA(delayA), delayC(delayC) {}
    };

    static vector<vector<MultiBase*>> fetchConfiguration(const std::string& configurationName, const std::string& boardName);
    static ConfigurationInfo parseConfigurationInfo(const std::string& configurationName, const vector<vector<MultiBase*>>& dbData);

    inline ConfigurationInfo getConfigurationInfo(const std::string& configurationName) const
    {
        auto dbData = fetchConfiguration(configurationName, m_boardName);
        return parseConfigurationInfo(configurationName, dbData);
    }

    // Due to lack of virtual inheritance for IndefiniteMapi, I don't think we can have BoardConfigurations inherit Mapi
    // And implement this method here
    virtual const string& getServiceName() const = 0;

    virtual ~BoardConfigurations() = default;

   protected:
    BoardCommunicationHandler m_handler;
    ConfigurationInfo m_configurationInfo;
    const string& m_boardName;

    BoardConfigurations(std::shared_ptr<Board> board) : m_handler(board), m_boardName(board->getName()) {}
};

class PmConfigurations : public Mapi, public BoardConfigurations
{
   public:
    PmConfigurations(std::shared_ptr<Board> board) : BoardConfigurations(board) {}
    string processInputMessage(string msg);
    string processOutputMessage(string msg);

    const string& getServiceName() const override { return name; }
};

class TcmConfigurations : public BasicFitIndefiniteMapi, public BoardConfigurations
{
   public:
    TcmConfigurations(std::shared_ptr<Board> board) : BoardConfigurations(board)
    {
        if (!board->doesExist(string(tcm_parameters::DelayA)) || !board->doesExist(string(tcm_parameters::DelayC))) {
            throw runtime_error("Couldn't construct TcmConfigurations: no delay parameters");
        }
    }

    void processExecution() override;

   private:
    string m_response;

    bool handleDelays();
    bool handleData();
    void handleResetErrors();

    const string& getServiceName() const override { return name; }
};