#pragma once
#include "Fred/Mapi/indefinitemapi.h"
#include "Board.h"
#include "database/DatabaseTables.h"
#include "Alfred/print.h"
#include "utils.h"
#include <unordered_set>
#include <functional>

class SaveConfiguration: public IndefiniteMapi
{
    public:
    /*  Expected line: [CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[VALUE]
    */
    SaveConfiguration(std::unordered_map<std::string, std::shared_ptr<Board>>& boards): m_Boards(boards) {
       connect("INSERT", wrapMemberFunction(this, &SaveConfiguration::constructInsert));
       connect("CREATE", wrapMemberFunction(this, &SaveConfiguration::constructCreate));
    }
    void processExecution() override;

    private:
    Result<std::string,std::string> substring(std::string_view sequence, size_t& start, char delimiter, std::function<bool(const std::string&)> validator, const std::string& errorMessage)
    {
        size_t stop = sequence.find(delimiter, start+1);
        if(stop == start || stop == (start + 1) || stop == std::string::npos){
            Print::PrintError(name, string_utils::concatenate("Empty substring"));
            return {.result=std::nullopt,.error="Empty substring"};
        }

        std::string strPhrase{sequence.substr(start,stop-start)};
        if(!validator(strPhrase)){
            Print::PrintError(name, string_utils::concatenate(errorMessage, strPhrase));
            return {.result=std::nullopt,.error=errorMessage};
        }
        start=stop+1;
        return {.result=std::move(strPhrase),.error=std::nullopt};
    }

    // Configuration name validator
    bool validateConfigurationName(const std::string& configurationName) 
    {
        return (m_knownConfigs.find(configurationName) != m_knownConfigs.end());
    }
    std::function<bool(const std::string&)> validatorConfigurationName = wrapMemberFunction(this, &SaveConfiguration::validateConfigurationName);

    // Board name validator
    bool validateBoardName(const std::string& boardName) 
    {
        return m_Boards.find(boardName) != m_Boards.end();
    }
    std::function<bool(const std::string&)> validatorBoardName = wrapMemberFunction(this, &SaveConfiguration::validateBoardName);

    // Command validator
    bool validateCmd(const std::string& cmdName)
    {
        return m_operations.find(cmdName) != m_operations.end();
    }
    std::function<bool(const std::string&)> validatorCommand = wrapMemberFunction(this, &SaveConfiguration::validateCmd);

    // No validation
    bool noValidation(const std::string&){
        return true;
    }
    std::function<bool(const std::string&)> alwaysValid = wrapMemberFunction(this, &SaveConfiguration::noValidation);

    void fetchAllConfigs();

    std::unordered_map<std::string, std::shared_ptr<Board>> m_Boards;
    std::unordered_set<std::string> m_knownConfigs;

    //

    typedef std::function<Result<std::string,std::string>(std::string_view)> QueryConstructor;

    std::unordered_map<std::string, QueryConstructor> m_operations;
    void connect(const std::string& operation, QueryConstructor queryConstructor){
        m_operations[operation] = queryConstructor;
    }

    Result<std::string,std::string> construct(std::string_view line, const std::string& cmd){
        return m_operations[cmd](line);
    }

    Result<std::string,std::string> constructCreate(std::string_view line);
    Result<std::string,std::string> constructInsert(std::string_view line);
};