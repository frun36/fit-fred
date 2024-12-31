#pragma once
#include "Fred/Mapi/indefinitemapi.h"
#include "Board.h"
#include "database/DatabaseTables.h"
#include "Alfred/print.h"
#include "utils.h"
#include <unordered_set>
#include <functional>

class ConfigurationDatabaseBroker: public IndefiniteMapi
{
    public:
    ConfigurationDatabaseBroker(std::unordered_map<std::string, std::shared_ptr<Board>>& boards): m_Boards(boards) {
        InsertParameter::Initialize(this);
        UpdateParameter::Initialize(this);
        SelectParameter::Initialize(this);
        SelectParameterVersion::Initialize(this);
        DeleteParameter::Initialize(this);
      
        InsertConfiguration::Initialize(this);
        SelectConfiguration::Initialize(this);
    }
    void processExecution() override;

    private:
    std::unordered_map<std::string, std::shared_ptr<Board>> m_Boards;

/*
    =   =   =   Constructors    =   =   =
    Constructor generates SQL query from WinCC request
*/

    typedef std::function<Result<std::string,std::string>(std::string_view)> QueryConstructor;
    std::unordered_map<std::string, QueryConstructor> m_constructors;

    void connect_constructor(const std::string& operation, QueryConstructor queryConstructor){
        m_constructors[operation] = queryConstructor;
    }

    Result<std::string,std::string> construct(std::string_view line, const std::string& cmd){
        return m_constructors[cmd](line);
    }

    Result<std::string,std::string> constructInsertConfiguration(std::string_view line);
    Result<std::string,std::string> constructInsertParameters(std::string_view line);
    Result<std::string,std::string> constructUpdateParameters(std::string_view line);
    Result<std::string,std::string> constructSelectParameters(std::string_view line);
    Result<std::string,std::string> constructSelectVersionsParameters(std::string_view line);
    Result<std::string,std::string> constructSelectConfiguration(std::string_view line);
    Result<std::string,std::string> constructDeleteParameter(std::string_view line);

/*
    =   =   =   Executors   =   =   =
    Executors execute SQL query and parses result to WinCC reponse format
*/

    typedef std::function<Result<std::string,std::string>(const std::string&)> QueryExecutor;
    std::unordered_map<std::string, QueryExecutor> m_executors;

    void connect_executor(const std::string& operation, QueryExecutor queryExecutor){
        m_executors[operation] = queryExecutor;
    }

    Result<std::string,std::string> execute(const std::string& query, const std::string& cmd)
    {
        return m_executors[cmd](query);
    }

    Result<std::string,std::string> executeUpdate(const std::string& query);
    Result<std::string,std::string> executeSelectParameters(const std::string& query);
    Result<std::string,std::string> executeSelectVerionsParameters(const std::string& query);
    Result<std::string,std::string> executeSelectConfiguration(const std::string& query);

/*
    =   =   =   Versions    =   =   =
    FDA-related utils (FDA - Flashback data archive)
*/
    std::string versions(const std::string& startDate)
    {
        return string_utils::concatenate(" VERSIONS BETWEEN timestamp to_timestamp('", startDate, "') and SYSTIMESTAMP ");
    }

    static constexpr std::string_view VersionColumns{"VERSIONS_STARTTIME,VERSIONS_ENDTIME,VERSIONS_OPERATION"};

/*
    =   =   = Request definition    =   =   =
*/

/*
    Add parameter to configuration:
        ->request: INSERT CONFIGURATION_PARAMETERS,[CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[PHYSICAL VALUE]
        ->response: -
*/

    struct InsertParameter
    {
        static constexpr size_t ExpectedSize = 4;
        static constexpr std::string_view Command = "INSERT CONFIGURATION_PARAMETERS";

        static void Initialize(ConfigurationDatabaseBroker*broker)
        {
            broker->connect_constructor(Command.data(), std::bind(&ConfigurationDatabaseBroker::constructInsertParameters,broker,std::placeholders::_1));
            broker->connect_executor(Command.data(), std::bind(&ConfigurationDatabaseBroker::executeUpdate,broker,std::placeholders::_1));
        }

        InsertParameter(const std::vector<std::string>& tokens):
            configName(tokens[0]),
            boardName(tokens[1]),
            paramName(tokens[2]),
            value(tokens[3])
        {
        }

        const std::string& configName;
        const std::string& boardName;
        const std::string& paramName;
        const std::string& value;
    };
/*
    Update parameter value:
        ->request: UPDATE CONFIGURATION_PARAMETERS,[CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[PHYSICAL VALUE]
        ->response: -
*/
    struct UpdateParameter
    {
        static constexpr size_t ExpectedSize = 4;
        static constexpr std::string_view Command = "UPDATE CONFIGURATION_PARAMETERS";

        static void Initialize(ConfigurationDatabaseBroker* broker)
        {
            broker->connect_constructor(Command.data(), std::bind(&ConfigurationDatabaseBroker::constructUpdateParameters,broker,std::placeholders::_1));
            broker->connect_executor(Command.data(), std::bind(&ConfigurationDatabaseBroker::executeUpdate,broker,std::placeholders::_1));
        }

        UpdateParameter(const std::vector<std::string>& tokens):
            configName(tokens[0]),
            boardName(tokens[1]),
            paramName(tokens[2]),
            value(tokens[3])
        {
        }
        const std::string& configName;
        const std::string& boardName;
        const std::string& paramName;
        const std::string& value;
    };

/*
    Fetch configuration parameters:
        -> request: SELECT CONFIGURATION_PARAMETERS,[CONFIGURATION NAME / *],[BOARD NAME / *],[PARAMETER NAME / *] 
        -> response: [CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[PHYSICAL VALUE]
*/
    struct SelectParameter
    {
        static constexpr size_t ExpectedSize = 3;
        static constexpr std::string_view Command = "SELECT CONFIGURATION_PARAMETERS";

        static void Initialize(ConfigurationDatabaseBroker* broker)
        {
            broker->connect_constructor(Command.data(), std::bind(&ConfigurationDatabaseBroker::constructSelectParameters,broker,std::placeholders::_1));
            broker->connect_executor(Command.data(), std::bind(&ConfigurationDatabaseBroker::executeSelectParameters,broker,std::placeholders::_1));
        }

        SelectParameter(const std::vector<std::string>& tokens):
            configName(tokens[0]),
            boardName(tokens[1]),
            paramName(tokens[2])
        {
        }
        const std::string& configName;
        const std::string& boardName;
        const std::string& paramName;
    };

/*
    Fetch configuration parameters versions:
        -> request: SELECT CONFIGURATION_PARAMETERS,[CONFIGURATION NAME / *],[BOARD NAME / *],[PARAMETER NAME / *] ,[STARTING DATE]
        -> response: [CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[PHYSICAL VALUE],[VERSIONS START DATE],[VERSIONS END DATE],[VERSIONS OPERATION] 
*/
    struct SelectParameterVersion
    {
        static constexpr size_t ExpectedSize = 4;
        static constexpr std::string_view Command = "SELECT VERSIONS CONFIGURATION_PARAMETERS";

        static void Initialize(ConfigurationDatabaseBroker* broker)
        {
            broker->connect_constructor(Command.data(), std::bind(&ConfigurationDatabaseBroker::constructSelectVersionsParameters,broker,std::placeholders::_1));
            broker->connect_executor(Command.data(), std::bind(&ConfigurationDatabaseBroker::executeSelectVerionsParameters,broker,std::placeholders::_1));
        }

        SelectParameterVersion(const std::vector<std::string>& tokens):
            configName(tokens[0]),
            boardName(tokens[1]),
            paramName(tokens[2]),
            startDate(tokens[3])
        {
        }
        const std::string& configName;
        const std::string& boardName;
        const std::string& paramName;
        const std::string& startDate;
    };
/*
    Fetch all configurations:
        -> request: SELECT CONFIGURATIONS,[CONFIGURATION NAME / *],[AUTHOR / *]
        -> response: [CONFIGURATION NAME],[AUTHOR],[DATE],[COMMENT]
*/
    struct SelectConfiguration
    {
        static constexpr size_t ExpectedSize = 2;
        static constexpr std::string_view Command = "SELECT CONFIGURATIONS";

        static void Initialize(ConfigurationDatabaseBroker* broker)
        {
            broker->connect_constructor(Command.data(), std::bind(&ConfigurationDatabaseBroker::constructSelectConfiguration,broker,std::placeholders::_1));
            broker->connect_executor(Command.data(),std::bind(&ConfigurationDatabaseBroker::executeSelectConfiguration,broker,std::placeholders::_1));
        }        

        SelectConfiguration(const std::vector<std::string>& tokens):
            configName(tokens[0]),
            author(tokens[1])
        {
        }
        const std::string& configName;
        const std::string& author;
    };

/*
    Create new configuration:
        -> request: INSERT CONFIGURATIONS,[CONFIGURATION NAME],[AUTHOR],[COMMENT]
        -> response: -
*/
    struct InsertConfiguration
    {
        static constexpr size_t ExpectedSize = 3;
        static constexpr std::string_view Command = "INSERT CONFIGURATIONS";

        static void Initialize(ConfigurationDatabaseBroker* broker)
        {
            broker->connect_constructor(Command.data(), std::bind(&ConfigurationDatabaseBroker::constructInsertConfiguration,broker,std::placeholders::_1));
            broker->connect_executor(Command.data(), std::bind(&ConfigurationDatabaseBroker::executeUpdate,broker,std::placeholders::_1));
        }

        InsertConfiguration(const std::vector<std::string>& tokens):
            configName(tokens[0]),
            author(tokens[1]),
            comment(tokens[2])
        {
        }
        const std::string& configName;
        const std::string& author;
        const std::string& comment;
    };

/*
    -> request: DELETE CONFIGURATION_PARAMETERS,[CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME]
*/
    struct DeleteParameter
    {
        static constexpr size_t ExpectedSize = 3;
        static constexpr std::string_view Command = "DELETE CONFIGURATION_PARAMETERS";

        static void Initialize(ConfigurationDatabaseBroker* broker)
        {
            broker->connect_constructor(Command.data(), std::bind(&ConfigurationDatabaseBroker::constructDeleteParameter,broker,std::placeholders::_1));
            broker->connect_executor(Command.data(), std::bind(&ConfigurationDatabaseBroker::executeUpdate, broker, std::placeholders::_1));
        }

        DeleteParameter(const std::vector<std::string>& tokens):
            configName(tokens[0]),
            boardName(tokens[1]),
            paramName(tokens[2])
        {     
        }

        const std::string& configName;
        const std::string& boardName;
        const std::string& paramName;
    };

};


