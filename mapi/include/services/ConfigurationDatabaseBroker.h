#pragma once
#include "Fred/Mapi/indefinitemapi.h"
#include "Board.h"
#include "database/DatabaseTables.h"
#include "Alfred/print.h"
#include "utils.h"
#include <unordered_set>
#include <functional>

/*
---- CONFIGURATION PARAMETERS TABLE ----

    ->Fetch configuration parameters:
        -> request: SELECT CONFIGURATION_PARAMETERS,[CONFIGURATION NAME / *],[BOARD NAME / *],[PARAMETER NAME / *] ( ,[STARTING DATE] )
        -> response: [CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[PHYSICAL VALUE] ( ,[VERSION START DATE],[VERSION END DATE] )

    ->Add parameter to configuration:
        ->request: INSERT CONFIGURATION_PARAMETERS,[CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[PHYSICAL VALUE]
        ->response: -

    ->Update parameter value:
        ->request: UPDATE CONFIGURATION_PARAMETERS,[CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[PHYSICAL VALUE]
        ->response: -

---- CONFIGURATION TABLE ----

    ->Create new configuration:
        ->request: INSERT CONFIGURATIONS,[CONFIGURATION NAME],[AUTHOR],[DATE],[COMMENT]
        ->response: -

    ->Fetch all configurations:
        ->request: SELECT CONFIGURATIONS,[CONFIGURATION NAME / *],[AUTHOR / *]
        ->response: [CONFIGURATION NAME],[AUTHOR],[DATE],[COMMENT]
*/


class ConfigurationDatabaseBroker: public IndefiniteMapi
{
    public:
    ConfigurationDatabaseBroker(std::unordered_map<std::string, std::shared_ptr<Board>>& boards): m_Boards(boards) {
    // CONFIGURATION_PARAMETERS
       connect_constructor("INSERT CONFIGURATION_PARAMETERS", wrapMemberFunction(this, &ConfigurationDatabaseBroker::constructInsertParameters));
       connect_constructor("UPDATE CONFIGURATION_PARAMETERS", wrapMemberFunction(this, &ConfigurationDatabaseBroker::constructUpdateParameters));
       connect_constructor("SELECT CONFIGURATION_PARAMETERS", wrapMemberFunction(this, &ConfigurationDatabaseBroker::constructSelectParameters));
       
       connect_executor("INSERT CONFIGURATION_PARAMETERS", wrapMemberFunction(this, &ConfigurationDatabaseBroker::executeUpdate));
       connect_executor("UPDATE CONFIGURATION_PARAMETERS", wrapMemberFunction(this, &ConfigurationDatabaseBroker::executeUpdate));
       connect_executor("SELECT CONFIGURATION_PARAMETERS", wrapMemberFunction(this, &ConfigurationDatabaseBroker::executeSelectParameters));
       
    // CONFIGURATION
       connect_constructor("INSERT CONFIGURATIONS", wrapMemberFunction(this, &ConfigurationDatabaseBroker::constructCreate));
       connect_executor("INSERT CONFIGURATIONS", wrapMemberFunction(this, &ConfigurationDatabaseBroker::executeUpdate));
    }
    void processExecution() override;

    private:
    

    // Configuration name validator
    bool validateConfigurationName(const std::string& configurationName) 
    {
        return (m_knownConfigs.find(configurationName) != m_knownConfigs.end());
    }
    std::function<bool(const std::string&)> validatorConfigurationName = wrapMemberFunction(this, &ConfigurationDatabaseBroker::validateConfigurationName);

    // Board name validator
    bool validateBoardName(const std::string& boardName) 
    {
        return m_Boards.find(boardName) != m_Boards.end();
    }
    std::function<bool(const std::string&)> validatorBoardName = wrapMemberFunction(this, &ConfigurationDatabaseBroker::validateBoardName);

    // Command validator
    bool validateCmd(const std::string& cmdName)
    {
        return m_constructors.find(cmdName) != m_constructors.end();
    }
    std::function<bool(const std::string&)> validatorCommand = wrapMemberFunction(this, &ConfigurationDatabaseBroker::validateCmd);

    // No validation
    bool noValidation(const std::string&){
        return true;
    }
    std::function<bool(const std::string&)> alwaysValid = wrapMemberFunction(this, &ConfigurationDatabaseBroker::noValidation);

    void fetchAllConfigs();

    std::unordered_map<std::string, std::shared_ptr<Board>> m_Boards;
    std::unordered_set<std::string> m_knownConfigs;

    //

    typedef std::function<Result<std::string,std::string>(std::string_view)> QueryConstructor;
    typedef std::function<Result<std::string,std::string>(const std::string&)> QueryExecutor;

    std::unordered_map<std::string, QueryConstructor> m_constructors;
    void connect_constructor(const std::string& operation, QueryConstructor queryConstructor){
        m_constructors[operation] = queryConstructor;
    }

    Result<std::string,std::string> construct(std::string_view line, const std::string& cmd){
        return m_constructors[cmd](line);
    }

    Result<std::string,std::string> constructCreate(std::string_view line);
    Result<std::string,std::string> constructInsertParameters(std::string_view line);
    Result<std::string,std::string> constructUpdateParameters(std::string_view line);
    Result<std::string,std::string> constructSelectParameters(std::string_view line);

    //
    std::unordered_map<std::string, QueryExecutor> m_executors;
    void connect_executor(const std::string& operation, QueryExecutor queryExecutor){
        m_executors[operation] = queryExecutor;
    }

    Result<std::string,std::string> executeUpdate(const std::string& query);
    Result<std::string,std::string> executeSelectParameters(const std::string& query);
    Result<std::string,std::string> execute(const std::string& query, const std::string& cmd)
    {
        return m_executors[cmd](query);
    }

    //

    std::string versions(const std::string& startDate)
    {
        return string_utils::concatenate("VERSIONS BETWEEN timestamp to_timestamp(", startDate, ") and SYSTIMESTAMP");
    }

    static constexpr std::string_view VersionColumns{"VERSIONS_STARTTIME,VERSIONS_ENDTIME,VERSIONS_OPERATION"};


};


