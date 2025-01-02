#include "services/ConfigurationDatabaseBroker.h"
#include "database/sql.h"
#include <limits>

#include <iomanip>

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

    size_t lineNumber = 0;
    std::string errorMessage;
    std::string queriesResult;
    bool success =  true;
    string_utils::Splitter splitter(request, '\n');

    do {
        std::string_view line = splitter.getNext();
        if(line.size() <= 1){
            Print::PrintWarning(name, "Line " + std::to_string(lineNumber) + " is empty. Skipping");
            continue;
        }

        size_t pos = 0;
        Result<std::string,std::string> result;
        std::string command;

        try{
            string_utils::Splitter splitter(line,',');
            command = splitter.getNext(pos);
            result = construct(line.substr(pos+1), command);
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

        Print::PrintData("Executing query: " + result.result.value());
        
        auto queryResult = execute(result.result.value(), command);
        if(queryResult.success() == false){
            success = false;
            errorMessage = queryResult.error.value();
            break;
        }
        if(queryResult.result.has_value()){
            Print::PrintData("Query result: " + queryResult.result.value());
            queriesResult.append(queryResult.result.value());
        }
        lineNumber++;

    }while(splitter.reachedEnd() == false);

    if(!success){
        DatabaseInterface::commitUpdate(false);
        std::string message = "Parsing request failed at line: " + std::to_string(lineNumber) + ". Message: " + errorMessage;
        Print::PrintError(name, message);
        publishError(message);
        return;
    }

    DatabaseInterface::commitUpdate(true);
    if(queriesResult.empty()){
        queriesResult = "Queries succeded";
    }
    publishAnswer(queriesResult);
}

/*
    =   =   =   Constructors    =   =   =
*/

Result<std::string,std::string> ConfigurationDatabaseBroker::constructInsertParameters(std::string_view line)
{
    auto result = string_utils::Splitter::getAll(line,',');
    if(result.success() == false){
        return {.result = std::nullopt, .error = result.error};
    }

    InsertParameter insert(result.result.value());
    
    if(m_Boards.find(insert.boardName) == m_Boards.end()){
        return {.result = std::nullopt, .error = "Invalid board name: " + insert.boardName};
    }
    auto& board = m_Boards[insert.boardName];
    if(board->doesExist(insert.paramName) == false){
        return {.result = std::nullopt, .error = "Invalid parameter name: " + insert.paramName};
    }

    std::string boradType = board->isTcm() ? "TCM" : "PM";

    double physcialValue = std::stod(insert.paramName);
    int64_t electronicValue = board->calculateElectronic(insert.paramName, physcialValue);

    sql::InsertModel query;
    query.insert(db_tables::ConfigurationParameters::ConfigurationName.name, insert.configName)
                (db_tables::ConfigurationParameters::BoardName.name, insert.boardName)
                (db_tables::ConfigurationParameters::BoardType.name, boradType)
                (db_tables::ConfigurationParameters::ParameterName.name, insert.paramName)
                (db_tables::ConfigurationParameters::ParameterValue.name, std::to_string(electronicValue)).into(db_tables::ConfigurationParameters::TableName);

    return {.result=query.str(), .error=std::nullopt};
}

Result<std::string,std::string> ConfigurationDatabaseBroker::constructInsertConfiguration(std::string_view line)
{
    //[CONFIGURATION NAME],[AUTHOR],[COMMENT]
    auto result = string_utils::Splitter::getAll(line,',');
    if(result.success() == false){
        return {.result = std::nullopt, .error = result.error};
    }
    if(result.result->size() != InsertConfiguration::ExpectedSize){
        return {.result = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.result->size())};
    }
    InsertConfiguration insert(result.result.value());

    sql::InsertModel query;
    query.insert(db_tables::Configurations::ConfigurationName.name, insert.configName)
                (db_tables::Configurations::Author.name, insert.author)
                (db_tables::Configurations::Comment.name, insert.comment).into(db_tables::Configurations::TableName);
    return {.result = query.str(), .error = std::nullopt};
}

Result<std::string,std::string> ConfigurationDatabaseBroker::constructUpdateParameters(std::string_view line)
{
    //[CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[PHYSICAL VALUE]
    auto result = string_utils::Splitter::getAll(line,',');

    if(result.success() == false){
        return {.result = std::nullopt, .error = result.error};
    }
    if(result.result->size() != UpdateParameter::ExpectedSize){
        return {.result = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.result->size())};
    }

    UpdateParameter update(result.result.value());
    
    if(m_Boards.find(update.boardName) == m_Boards.end()){
        return {.result = std::nullopt, .error = "Invalid board name: " + update.boardName};
    }
    auto& board = m_Boards[update.boardName];
    if(board->doesExist(update.paramName) == false){
        return {.result = std::nullopt, .error = "Invalid parameter name: " + update.paramName};
    }

    std::string boradType = board->isTcm() ? "TCM" : "PM";

    double physcialValue = std::stod(update.value);
    int64_t electronicValue = board->calculateElectronic(update.paramName, physcialValue);

    sql::UpdateModel query;
    query.update(db_tables::ConfigurationParameters::TableName).set(db_tables::ConfigurationParameters::ParameterValue.name, std::to_string(electronicValue)).where(
        sql::column(db_tables::ConfigurationParameters::ConfigurationName.name) == update.configName &&
        sql::column(db_tables::ConfigurationParameters::BoardType.name) == boradType &&
        sql::column(db_tables::ConfigurationParameters::BoardName.name) == update.boardName &&
        sql::column(db_tables::ConfigurationParameters::ParameterName.name) == update.paramName
        );
    return {.result = query.str(), .error = std::nullopt};
}

Result<std::string,std::string> ConfigurationDatabaseBroker::constructSelectParameters(std::string_view line)
{
    //[CONFIGURATION NAME / *],[BOARD NAME / *],[PARAMETER NAME / *] ( ,[STARTING DATE] )
    auto result = string_utils::Splitter::getAll(line,',');
    if(result.success() == false){
        return {.result = std::nullopt, .error = result.error};
    }
    if(result.result->size() != SelectParameter::ExpectedSize){
        return {.result = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.result->size())};
    }
    SelectParameter select(result.result.value());
    sql::SelectModel query;
    query.select("*").from(db_tables::ConfigurationParameters::TableName);
    if(select.configName != "*"){
        query.where(sql::column(db_tables::ConfigurationParameters::ConfigurationName.name) == select.configName);
    }
    if(select.boardName != "*"){
        query.where(sql::column(db_tables::ConfigurationParameters::BoardName.name) == select.boardName);
    }
    if(select.paramName != "*"){
        query.where(sql::column(db_tables::ConfigurationParameters::ParameterName.name) == select.paramName);
    }
    return {.result = query.str(), .error = std::nullopt};
}

Result<std::string,std::string> ConfigurationDatabaseBroker::constructSelectVersionsParameters(std::string_view line)
{
    auto result = string_utils::Splitter::getAll(line,',');
    if(result.success() == false){
        return {.result = std::nullopt, .error = result.error};
    }
    if(result.result->size() != SelectParameterVersion::ExpectedSize){
        return {.result = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.result->size())};
    }
    SelectParameterVersion select(result.result.value());
    sql::SelectModel query;

    query.select("*").from(db_tables::ConfigurationParameters::TableName);
    if(select.configName != "*"){
        query.where(sql::column(db_tables::ConfigurationParameters::ConfigurationName.name) == select.configName);
    }
    if(select.boardName != "*"){
        query.where(sql::column(db_tables::ConfigurationParameters::BoardName.name) == select.boardName);
    }
    if(select.paramName != "*"){
        query.where(sql::column(db_tables::ConfigurationParameters::ParameterName.name) == select.paramName);
    }
    std::string queryStr = query.str();
    queryStr.insert(queryStr.find("*"),string_utils::concatenate(" ",db_tables::ConfigurationParameters::TableName,"."));
    queryStr.insert(queryStr.find("*")+1,string_utils::concatenate(",",VersionColumns));
    std::string version = versions(select.startDate);
    size_t versionPos = queryStr.find("where");
    if(versionPos != std::string::npos){
        queryStr.insert(versionPos-1, version);
    }
    else{
        queryStr.append(version);
    }
    return {.result = std::move(queryStr), .error = std::nullopt};
}

Result<std::string,std::string> ConfigurationDatabaseBroker::constructSelectConfiguration(std::string_view line)
{
    auto result = string_utils::Splitter::getAll(line,',');
    if(result.success() == false){
        return {.result = std::nullopt, .error = result.error};
    }
    if(result.result->size() != SelectConfiguration::ExpectedSize){
        return {.result = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.result->size())};
    }
    SelectConfiguration select( result.result.value());

    sql::SelectModel query;
    query.select("*").from(db_tables::Configurations::TableName);
    if(select.configName != "*"){
        query.where(sql::column(db_tables::ConfigurationParameters::ConfigurationName.name) == select.configName);
    }
    if(select.author != "*"){
        query.where(sql::column(db_tables::Configurations::Author.name) == select.author);
    }

    return {.result=query.str(),.error=std::nullopt};
}

Result<std::string,std::string> ConfigurationDatabaseBroker::constructDeleteParameter(std::string_view line)
{
    auto result = string_utils::Splitter::getAll(line,',');
    if(result.success() == false){
        return {.result = std::nullopt, .error = result.error};
    }
    if(result.result->size() != DeleteParameter::ExpectedSize){
        return {.result = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.result->size())};
    }
    DeleteParameter deletion( result.result.value());

    sql::DeleteModel query;
    query.from(db_tables::ConfigurationParameters::TableName);
    query.where(sql::column(db_tables::ConfigurationParameters::ConfigurationName.name) == deletion.configName);
    query.where(sql::column(db_tables::ConfigurationParameters::BoardName.name) == deletion.boardName);
    query.where(sql::column(db_tables::ConfigurationParameters::ParameterName.name) == deletion.paramName);
    return {.result=query.str(), .error = std::nullopt};
}


/*
    =   =   =   Executors   =   =   =
*/

Result<std::string,std::string> ConfigurationDatabaseBroker::executeSelectParameters(const std::string& query)
{
    std::string rows;
    bool status = false;
    auto results = DatabaseInterface::executeQuery(query,status);
    auto toPhysical = [this](const std::string& boardName, const std::string& parameterName, int64_t electronicValue){
        double physicalValue = m_Boards[boardName]->calculatePhysical(parameterName,electronicValue);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(3) << physicalValue;
        return ss.str();
    };

    if(!status){
        return {.result = std::nullopt, .error = "SELECT query failed: " + query};
    }

    if(results.empty()){
        return {.result = rows, .error = std::nullopt};
    }

    for(auto& row: results){
        std::string boardName = row[db_tables::ConfigurationParameters::BoardName.idx]->getString();
        std::string parameterName = row[db_tables::ConfigurationParameters::ParameterName.idx]->getString();
        std::string value = toPhysical(boardName, parameterName, row[db_tables::ConfigurationParameters::ParameterValue.idx]->getDouble());

        rows += row[db_tables::ConfigurationParameters::ConfigurationName.idx]->getString() + ",";
        rows += boardName + ",";
        rows += parameterName + ",";
        rows += value;
        rows += "\n";
        }
    return {.result = rows, .error = std::nullopt};
}

Result<std::string,std::string> ConfigurationDatabaseBroker::executeSelectVerionsParameters(const std::string& query)
{
    std::string rows;
    bool status = false;
    auto results = DatabaseInterface::executeQuery(query,status);
    auto toPhysical = [this](const std::string& boardName, const std::string& parameterName, int64_t electronicValue){
        double physicalValue = m_Boards[boardName]->calculatePhysical(parameterName,electronicValue);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(3) << physicalValue;
        return ss.str();
    };

    if(!status){
        return {.result = std::nullopt, .error = "SELECT query failed: " + query};
    }

    if(results.empty()){
        return {.result = rows, .error = std::nullopt};
    }

    for(auto& row: results){
        std::string boardName = row[db_tables::ConfigurationParameters::BoardName.idx]->getString();
        std::string parameterName = row[db_tables::ConfigurationParameters::ParameterName.idx]->getString();
        std::string value = toPhysical(boardName, parameterName, row[db_tables::ConfigurationParameters::ParameterValue.idx]->getDouble());

        rows += row[db_tables::ConfigurationParameters::ConfigurationName.idx]->getString() + ",";
        rows += boardName + ",";
        rows += parameterName + ",";
        rows += value + ",";
        rows += (row[5] != nullptr) ? (row[5]->getString() + ",") : "NULL,";
        rows += (row[6] != nullptr) ? (row[6]->getString() + ",") : "NULL,";
        rows += (row[7] != nullptr) ? (row[7]->getString() + ",") : "NULL,";
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


Result<std::string,std::string> ConfigurationDatabaseBroker::executeSelectConfiguration(const std::string& query)
{
    std::string rows;
    bool status = false;
    auto results = DatabaseInterface::executeQuery(query,status);

    if(!status){
        return {.result = std::nullopt, .error = "SELECT query failed: " + query};
    }

    if(results.empty()){
        return {.result = rows, .error = std::nullopt};
    }

    for(auto& row: results){
        rows += row[db_tables::Configurations::ConfigurationName.idx]->getString() + ",";
        rows += row[db_tables::Configurations::Author.idx]->getString() + ",";
        rows += row[db_tables::Configurations::Date.idx]->getString() + ",";
        rows += row[db_tables::Configurations::Comment.idx]->getString();
        rows += "\n";
    }
    return {.result = rows, .error = std::nullopt};
}

