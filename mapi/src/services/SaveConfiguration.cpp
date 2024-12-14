#include "services/SaveConfiguration.h"
#include "database/sql.h"
#include <limits>

void SaveConfiguration::fetchAllConfigs()
{
    m_knownConfigs.clear();
    sql::SelectModel query;
    query.select(db_tables::Configurations::ConfigurationName.name).from(db_tables::Configurations::TableName);
    auto result = DatabaseInterface::executeQuery(query.str());
    for(auto& row: result){
        m_knownConfigs.emplace(row[0]->getString());
    }
}

Result<std::string,std::string> SaveConfiguration::constructInsert(std::string_view line)
{
    size_t start = 0;

    std::string configurationName;
    {
        auto parsingResult = substring(line,start,',',validatorConfigurationName, "Unknown configuration: ");
        if(!parsingResult.success()){
            return parsingResult;
        }
        configurationName = parsingResult.result.value();
    }

    std::string boardName;
    {
        auto parsingResult = substring(line,start,',',validatorBoardName, "Unknown board: ");
        if(!parsingResult.success()){
            return parsingResult;
        }
        boardName = parsingResult.result.value();
    }

    auto boardItr = m_Boards.find(boardName);
    auto board = boardItr->second;
    std::string_view boradType = board->isTcm() ? "TCM" : "PM";

    std::string parameterName;
    {
        auto parsingResult = substring(line, start, ',', 
                            [&board](const std::string&parameterName){return board->doesExist(parameterName);},
                            "Unknown parameter: ");
        if(!parsingResult.success()){
            return parsingResult;
        }
        parameterName = parsingResult.result.value();
    }

    double physcialValue = std::stod(line.substr(start).data());
    int64_t electronicValue = board->calculateElectronic(parameterName.data(), physcialValue);

    sql::InsertModel query;
    query.insert(db_tables::ConfigurationParameters::ConfigurationName.name, configurationName)
                (db_tables::ConfigurationParameters::BoardName.name, boardName)
                (db_tables::ConfigurationParameters::BoardType.name, boradType.data())
                (db_tables::ConfigurationParameters::ParameterName.name, parameterName)
                (db_tables::ConfigurationParameters::ParameterValue.name, std::to_string(electronicValue)).into(db_tables::ConfigurationParameters::TableName);

    return {.result=query.str(), .error=std::nullopt};
}

Result<std::string,std::string> SaveConfiguration::constructCreate(std::string_view line)
{
    size_t start = 0;
    std::string configurationName;
    {
        auto parsingResult = substring(line,start,',',alwaysValid, {});
        if(!parsingResult.success()){
            return parsingResult;
        }
        configurationName = parsingResult.result.value();
    }

    std::string author;
    {
        auto parsingResult = substring(line,start,',',alwaysValid,{});
        if(!parsingResult.success()){
            return parsingResult;
        }
        author = parsingResult.result.value();
    }

    std::string date;
    {
        auto parsingResult = substring(line,start,',',alwaysValid,{});
        if(!parsingResult.success()){
            return parsingResult;
        }
        date = parsingResult.result.value();
    }

    std::string comment{ line.substr(start).data() };

    m_knownConfigs.emplace(configurationName);

    sql::InsertModel query;
    query.insert(db_tables::Configurations::ConfigurationName.name, configurationName)
                (db_tables::Configurations::Author.name, author)
                (db_tables::Configurations::Date.name, date)
                (db_tables::Configurations::Comment.name, comment).into(db_tables::Configurations::TableName);
    return {.result = query.str(), .error = std::nullopt};
}

Result<std::string,std::string> SaveConfiguration::constructUpdate(std::string_view line)
{
     size_t start = 0;

    std::string configurationName;
    {
        auto parsingResult = substring(line,start,',',validatorConfigurationName, "Unknown configuration: ");
        if(!parsingResult.success()){
            return parsingResult;
        }
        configurationName = parsingResult.result.value();
    }

    std::string boardName;
    {
        auto parsingResult = substring(line,start,',',validatorBoardName, "Unknown board: ");
        if(!parsingResult.success()){
            return parsingResult;
        }
        boardName = parsingResult.result.value();
    }

    auto boardItr = m_Boards.find(boardName);
    auto board = boardItr->second;
    std::string boradType = board->isTcm() ? "TCM" : "PM";

    std::string parameterName;
    {
        auto parsingResult = substring(line, start, ',', 
                            [&board](const std::string&parameterName){return board->doesExist(parameterName);},
                            "Unknown parameter: ");
        if(!parsingResult.success()){
            return parsingResult;
        }
        parameterName = parsingResult.result.value();
    }

    double physcialValue = std::stod(line.substr(start).data());
    int64_t electronicValue = board->calculateElectronic(parameterName.data(), physcialValue);

    sql::UpdateModel query;
    query.update(db_tables::ConfigurationParameters::TableName).set(db_tables::ConfigurationParameters::ParameterValue.name, std::to_string(electronicValue)).where(
        sql::column(db_tables::ConfigurationParameters::ConfigurationName.name) == configurationName &&
        sql::column(db_tables::ConfigurationParameters::BoardType.name) == boradType &&
        sql::column(db_tables::ConfigurationParameters::BoardName.name) == boardName &&
        sql::column(db_tables::ConfigurationParameters::ParameterName.name) == parameterName
        );
    return {.result = query.str(), .error = std::nullopt};
}

void SaveConfiguration::processExecution()
{
    bool running = true;
    std::string request = waitForRequest(running);
    if(!running){
        return;
    }

    if(request.empty()){
        Print::PrintError(name, "Empty request!");
        publishError("Empty request!");
    }
    if(DatabaseInterface::isConnected() == false){
        Print::PrintError(name, "FRED is not connected to the database!");
        publishError("FRED is not connected to the database!");
    }

    fetchAllConfigs();

    size_t lineBeg = 0;
    size_t lineEnd = std::numeric_limits<size_t>::max();
    size_t lineNumber = 0;
    std::string errorMessage;
    std::string queriesResult;
    bool success =  true;
    
    do {
        lineBeg = lineEnd+1;
        lineEnd = request.find('\n',lineBeg);
        if(lineEnd - lineBeg == 0){
            Print::PrintWarning(name, "Line " + std::to_string(lineNumber) + " is empty. Skipping");
            continue;
        }
        size_t start = 0;
        std::string_view line = std::string_view(&request[lineBeg], lineEnd-lineBeg);
        Result<std::string,std::string> result;
        std::string command;

        try{
            auto cmdParsingResult = substring(line,start,',',validatorCommand,"Invalid command");
            if(cmdParsingResult.success() == false){
                errorMessage = result.error.value();
                success = false;
                break;
            }
            command = cmdParsingResult.result.value();
            result = construct(line.substr(start), command);
        }
        catch(std::runtime_error& err){
            result = {.result=std::nullopt, .error = err.what()};
        }
        catch(std::exception& err){
            result = {.result=std::nullopt, .error = err.what()};
        }

        if(!result.success()){
            errorMessage = result.error.value();
            success = false;
            break;
        }

        auto queryResult = execute(result.result.value(), command);
        if(queryResult.success() == false){
            success = false;
            errorMessage = queryResult.error.value();
            break;
        }
        if(queryResult.result.has_value()){
            queriesResult.append(queryResult.result.value());
        }
        lineNumber++;

    }while(lineEnd != std::string::npos);

    if(!success){
        DatabaseInterface::commitUpdate(false);
        std::string message = "Parsing request failed at line: " + std::to_string(lineNumber) + ". Message: " + errorMessage;
        Print::PrintError(name, message);
        publishError(message);
        return;
    }

    DatabaseInterface::commitUpdate(true);
    if(queriesResult.empty()){
        queriesResult = "Updated database successfully";
    }
    publishAnswer(queriesResult);
}

Result<std::string,std::string> SaveConfiguration::executeSelectParameters(const std::string& query)
{
    std::string rows;
    bool status = false;
    auto results = DatabaseInterface::executeQuery(query,status);

    if(!status){
        return {.result = std::nullopt, .error = "SELECT query failed: " + query};
    }

    for(auto& row: results){
        rows += row[db_tables::ConfigurationParameters::ConfigurationName.idx]->getString() + ",";
        rows += row[db_tables::ConfigurationParameters::BoardName.idx]->getString() + ",";
        rows += row[db_tables::ConfigurationParameters::BoardType.idx]->getString() + ",";
        rows += row[db_tables::ConfigurationParameters::ParameterName.idx]->getString() + ",";
        rows += row[db_tables::ConfigurationParameters::ParameterValue.idx]->getString() + ",";
        rows += "\n";
    }
    return {.result = rows, .error = std::nullopt};
}

Result<std::string,std::string> SaveConfiguration::executeUpdate(const std::string& query)
{
    std::string errorMessage;
    DatabaseInterface::executeUpdate(query,errorMessage);
    if(errorMessage.empty() == false){
        return {.result = std::nullopt, .error = errorMessage};
    }
    return {.result = std::nullopt, .error = std::nullopt};
}