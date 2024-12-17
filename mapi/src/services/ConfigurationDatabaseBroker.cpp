#include "services/ConfigurationDatabaseBroker.h"
#include "database/sql.h"
#include <limits>

#include <iomanip>


Result<std::string,std::string> substring(std::string_view sequence, size_t& start, char delimiter, std::function<bool(const std::string&)> validator, const std::string& errorMessage)
{
    if(start == std::string::npos || start > sequence.size()){
        return {.result=std::nullopt,.error="Out of bound, tried to access string at pos " + std::to_string(start) + " (size: " + std::to_string(sequence.size()) +")"};
    }

    size_t stop = sequence.find(delimiter, start+1);
    if(stop == std::string::npos){
        stop = sequence.size();
    }

    if(stop == start){
        return {.result=std::nullopt,.error="Empty substring"};
    }

    std::string strPhrase{sequence.substr(start,stop-start)};
    if(!validator(strPhrase)){
        return {.result=std::nullopt,.error=errorMessage};
    }
    start=stop+1;
    return {.result=std::move(strPhrase),.error=std::nullopt};
}

template <typename... Validators>
Result<std::vector<std::string>, std::string> parse(std::string_view line, bool isPartialValid, Validators&... validators) {
    static_assert((std::is_invocable_r<bool,Validators,const std::string&>::value && ...), "All validators must be bool(const std::string&) callables");

    std::vector<std::string> results;
    results.reserve(sizeof...(Validators));

    size_t pos = 0;
    std::string errorMessage;

    auto process_validator = [&](auto& validator) {
        if (!errorMessage.empty()) return;

        auto parsingResult = substring(line, pos, ',', validator, "Invalid token: ");
        if (!parsingResult.success()) {
            errorMessage = parsingResult.error.value();
        } else {
            results.push_back(*parsingResult.result);
        }
    };

    (process_validator(validators), ...); // Fold expression

    if (!errorMessage.empty()) {
        return {.result=std::move(results), .error=errorMessage};
    }
    if(!isPartialValid && results.size() < (sizeof...(Validators))){
        return {.result=std::nullopt, .error="Missing tokens: " + std::to_string(results.size()) + " parsed, expected " + std::to_string(sizeof...(Validators))};
    }
    return {.result=std::move(results),.error=std::nullopt};
}

/// 

void ConfigurationDatabaseBroker::fetchAllConfigs()
{
    m_knownConfigs.clear();
    sql::SelectModel query;
    query.select(db_tables::Configurations::ConfigurationName.name).from(db_tables::Configurations::TableName);
    auto result = DatabaseInterface::executeQuery(query.str());
    for(auto& row: result){
        m_knownConfigs.emplace(row[0]->getString());
    }
}

Result<std::string,std::string> ConfigurationDatabaseBroker::constructInsertParameters(std::string_view line)
{
    //[CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[PHYSICAL VALUE]
    auto result = parse(line, false, validatorConfigurationName, validatorBoardName, alwaysValid, alwaysValid);
    if(result.success() == false){
        return {.result = std::nullopt, .error = result.error};
    }

    const auto& tokens = result.result.value();
    const std::string& configurationName = tokens[0];
    const std::string& boardName = tokens[1];
    const std::string& parameterName = tokens[2];
    const std::string& parameterValue = tokens[3];
    
    auto& board = m_Boards[boardName];
    if(board->doesExist(parameterName) == false){
        return {.result = std::nullopt, .error = "Invalid parameter name: " + parameterName};
    }

    std::string boradType = board->isTcm() ? "TCM" : "PM";

    double physcialValue = std::stod(parameterValue);
    int64_t electronicValue = board->calculateElectronic(parameterName.data(), physcialValue);

    sql::InsertModel query;
    query.insert(db_tables::ConfigurationParameters::ConfigurationName.name, configurationName)
                (db_tables::ConfigurationParameters::BoardName.name, boardName)
                (db_tables::ConfigurationParameters::BoardType.name, boradType)
                (db_tables::ConfigurationParameters::ParameterName.name, parameterName)
                (db_tables::ConfigurationParameters::ParameterValue.name, std::to_string(electronicValue)).into(db_tables::ConfigurationParameters::TableName);

    return {.result=query.str(), .error=std::nullopt};
}

Result<std::string,std::string> ConfigurationDatabaseBroker::constructCreate(std::string_view line)
{
    //[CONFIGURATION NAME],[AUTHOR],[DATE],[COMMENT]
    auto result = parse(line, false, alwaysValid, alwaysValid, alwaysValid, alwaysValid);
    if(result.success() == false){
        return {.result = std::nullopt, .error = result.error};
    }
    const auto& tokens = result.result.value();
    const std::string& configurationName = tokens[0];
    const std::string& author = tokens[1];
    const std::string& date = tokens[2];
    const std::string& comment = tokens[3];

    m_knownConfigs.emplace(configurationName);

    sql::InsertModel query;
    query.insert(db_tables::Configurations::ConfigurationName.name, configurationName)
                (db_tables::Configurations::Author.name, author)
                (db_tables::Configurations::Date.name, date)
                (db_tables::Configurations::Comment.name, comment).into(db_tables::Configurations::TableName);
    return {.result = query.str(), .error = std::nullopt};
}

Result<std::string,std::string> ConfigurationDatabaseBroker::constructUpdateParameters(std::string_view line)
{
    //[CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[PHYSICAL VALUE]
    auto result = parse(line, false, validatorConfigurationName, validatorBoardName, alwaysValid, alwaysValid);
    if(result.success() == false){
        return {.result = std::nullopt, .error = result.error};
    }

    const auto& tokens = result.result.value();
    const std::string& configurationName = tokens[0];
    const std::string& boardName = tokens[1];
    const std::string& parameterName = tokens[2];
    const std::string& parameterValue = tokens[3];
    
    auto& board = m_Boards[boardName];
    if(board->doesExist(parameterName) == false){
        return {.result = std::nullopt, .error = "Invalid parameter name: " + parameterName};
    }

    std::string boradType = board->isTcm() ? "TCM" : "PM";

    double physcialValue = std::stod(parameterValue);
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

Result<std::string,std::string> ConfigurationDatabaseBroker::constructSelectParameters(std::string_view line)
{
    //[CONFIGURATION NAME / *],[BOARD NAME / *],[PARAMETER NAME / *] ( ,[STARTING DATE] )
    auto result = parse(line, true, alwaysValid, alwaysValid, alwaysValid, alwaysValid);
    if(result.success() == false){
        return {.result = std::nullopt, .error = result.error};
    }

    const auto& tokens = result.result.value();
    const std::string& configurationName = tokens[0];
    const std::string& boardName = tokens[1];
    const std::string& parameterName = tokens[2];
    std::string startDate = (tokens.size() == 4) ? tokens[3] : std::string();
   
    sql::SelectModel query;
    bool where = false;
    query.select("*").from(db_tables::ConfigurationParameters::TableName);
    if(configurationName != "*"){
        where = true;
        query.where(sql::column(db_tables::ConfigurationParameters::ConfigurationName.name) == configurationName);
    }
    if(boardName != "*"){
        where=true;
        query.where(sql::column(db_tables::ConfigurationParameters::BoardName.name) == boardName);
    }
    if(parameterName != "*"){
        where=true;
        query.where(sql::column(db_tables::ConfigurationParameters::ParameterName.name) == parameterName);
    }

    std::string queryStr = query.str();

    if(startDate.length() && where)
    {
        std::string version = versions(startDate);
        queryStr.insert(queryStr.find("where")-1, version);
    }
    else if(startDate.length())
    {
        std::string version = versions(startDate);
        queryStr.append(version);
    }

    return {.result = std::move(queryStr), .error = std::nullopt};
}

void ConfigurationDatabaseBroker::processExecution()
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
        lineEnd = (lineEnd != std::string::npos) ? lineEnd : request.size();
        if(lineEnd - lineBeg <= 1){
            Print::PrintWarning(name, "Line " + std::to_string(lineNumber) + " is empty. Skipping");
            continue;
        }
        size_t pos = 0;
        std::string_view line = std::string_view(&request[lineBeg], lineEnd-lineBeg);
        Result<std::string,std::string> result;
        std::string command;

        try{
            auto cmdParsingResult = substring(line,pos,',',validatorCommand,"Invalid command");
            if(cmdParsingResult.success() == false){
                errorMessage = cmdParsingResult.error.value();
                success = false;
                break;
            }
            command = cmdParsingResult.result.value();
            result = construct(line.substr(pos), command);
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

    }while(lineEnd < request.size());

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

Result<std::string,std::string> ConfigurationDatabaseBroker::executeSelectParameters(const std::string& query)
{
    std::string rows;
    bool status = false;
    auto results = DatabaseInterface::executeQuery(query,status);

    if(!status){
        return {.result = std::nullopt, .error = "SELECT query failed: " + query};
    }

    for(auto& row: results){
        std::string boardName = row[db_tables::ConfigurationParameters::BoardName.idx]->getString();
        std::string parameterName = row[db_tables::ConfigurationParameters::ParameterName.idx]->getString();
        int64_t electronicValue = std::stoll(row[db_tables::ConfigurationParameters::ParameterValue.idx]->getString());
        double physicalValue = m_Boards[boardName]->calculatePhysical(parameterName,electronicValue);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(3) << physicalValue;

        rows += row[db_tables::ConfigurationParameters::ConfigurationName.idx]->getString() + ",";
        rows += boardName + ",";
        rows += parameterName + ",";
        rows += ss.str();
        rows += "\n";
    }
    return {.result = rows, .error = std::nullopt};
}

Result<std::string,std::string> ConfigurationDatabaseBroker::executeUpdate(const std::string& query)
{
    std::string errorMessage;
    DatabaseInterface::executeUpdate(query,errorMessage);
    if(errorMessage.empty() == false){
        return {.result = std::nullopt, .error = errorMessage};
    }
    return {.result = std::nullopt, .error = std::nullopt};
}