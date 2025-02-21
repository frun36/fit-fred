#include "services/configurations/ConfigurationDatabaseBroker.h"
#include "Alfred/print.h"
#include "Database/databaseinterface.h"
#include "database/sql.h"
#include "database/DatabaseViews.h"
#include <sstream>
#include <iomanip>

void ConfigurationDatabaseBroker::processExecution()
{
    bool running = true;
    std::string request = waitForRequest(running);
    if (!running) {
        return;
    }

    if (request.empty()) {
        Print::PrintError(name, "Empty request!");
        publishError("Empty request!");
    }
    if (DatabaseInterface::isConnected() == false) {
        Print::PrintError(name, "FRED is not connected to the database!");
        publishError("FRED is not connected to the database!");
    }

    size_t lineNumber = 0;
    std::string errorMessage;
    std::string queriesResult;
    bool success = true;
    string_utils::Splitter splitter(request, '\n');

    do {
        std::string_view line = splitter.getNext();
        if (line.empty()) {
            Print::PrintWarning(name, "Line " + std::to_string(lineNumber) + " is empty. Skipping");
            continue;
        }

        size_t pos = 0;
        Result<std::string, std::string> result;
        std::string command;

        try {
            string_utils::Splitter splitter(line, ',');
            command = splitter.getNext(pos);
            result = construct(line.substr(pos + 1), command);
        } catch (std::runtime_error& err) {
            result = { .ok = std::nullopt, .error = err.what() };
        } catch (std::exception& err) {
            result = { .ok = std::nullopt, .error = err.what() };
        }

        if (!result.isOk()) {
            errorMessage = result.error.value();
            success = false;
            break;
        }

        Print::PrintData("Executing query: " + result.ok.value());

        auto queryResult = execute(result.ok.value(), command);
        if (queryResult.isOk() == false) {
            success = false;
            errorMessage = queryResult.error.value();
            break;
        }
        if (queryResult.ok.has_value()) {
            Print::PrintData("Query result: " + queryResult.ok.value());
            queriesResult.append(queryResult.ok.value());
        }
        lineNumber++;

    } while (splitter.reachedEnd() == false);

    if (!success) {
        DatabaseInterface::commitUpdate(false);
        std::string message = "Parsing request failed at line: " + std::to_string(lineNumber) + ". Message: " + errorMessage;
        Print::PrintError(name, message);
        publishError(message);
        return;
    }

    DatabaseInterface::commitUpdate(true);
    if (queriesResult.empty()) {
        queriesResult = "Queries succeeded";
    }
    publishAnswer(queriesResult);
}

/*
    =   =   =   Constructors    =   =   =
*/

Result<std::string, std::string> ConfigurationDatabaseBroker::constructInsertParameters(std::string_view line)
{
    auto result = string_utils::Splitter::getAll(line, ',');
    if (result.isOk() == false) {
        return { .ok = std::nullopt, .error = result.error };
    }
    if (result.ok->size() != InsertParameter::ExpectedSize) {
        return { .ok = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.ok->size()) };
    }
    InsertParameter insert(result.ok.value());

    if (m_Boards.find(insert.boardName) == m_Boards.end()) {
        return { .ok = std::nullopt, .error = "Invalid board name: " + insert.boardName };
    }
    auto& board = m_Boards[insert.boardName];
    if (board->doesExist(insert.paramName) == false) {
        return { .ok = std::nullopt, .error = "Invalid parameter name: " + insert.paramName };
    }

    std::string boardType = board->isTcm() ? "TCM" : "PM";

    double physicalValue = std::stod(insert.value);
    int64_t electronicValue = board->calculateElectronic(insert.paramName, physicalValue);

    sql::InsertModel query;
    query.insert(db_fit::tables::ConfigurationParameters::ConfigurationName.name, insert.configName)(db_fit::tables::ConfigurationParameters::BoardName.name, insert.boardName)(db_fit::tables::ConfigurationParameters::BoardType.name, boardType)(db_fit::tables::ConfigurationParameters::ParameterName.name, insert.paramName)(db_fit::tables::ConfigurationParameters::ParameterValue.name, std::to_string(electronicValue)).into(db_fit::tables::ConfigurationParameters::TableName);

    return { .ok = query.str(), .error = std::nullopt };
}

Result<std::string, std::string> ConfigurationDatabaseBroker::constructInsertConfiguration(std::string_view line)
{
    //[CONFIGURATION NAME],[AUTHOR],[COMMENT]
    auto result = string_utils::Splitter::getAll(line, ',');
    if (result.isOk() == false) {
        return { .ok = std::nullopt, .error = result.error };
    }
    if (result.ok->size() != InsertConfiguration::ExpectedSize) {
        return { .ok = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.ok->size()) };
    }
    InsertConfiguration insert(result.ok.value());

    sql::InsertModel query;
    query.insert(db_fit::tables::Configurations::ConfigurationName.name, insert.configName)(db_fit::tables::Configurations::Author.name, insert.author)(db_fit::tables::Configurations::Comment.name, insert.comment).into(db_fit::tables::Configurations::TableName);
    return { .ok = query.str(), .error = std::nullopt };
}

Result<std::string, std::string> ConfigurationDatabaseBroker::constructUpdateParameters(std::string_view line)
{
    //[CONFIGURATION NAME],[BOARD NAME],[PARAMETER NAME],[PHYSICAL VALUE]
    auto result = string_utils::Splitter::getAll(line, ',');

    if (result.isOk() == false) {
        return { .ok = std::nullopt, .error = result.error };
    }
    if (result.ok->size() != UpdateParameter::ExpectedSize) {
        return { .ok = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.ok->size()) };
    }

    UpdateParameter update(result.ok.value());

    if (m_Boards.find(update.boardName) == m_Boards.end()) {
        return { .ok = std::nullopt, .error = "Invalid board name: " + update.boardName };
    }
    auto& board = m_Boards[update.boardName];
    if (board->doesExist(update.paramName) == false) {
        return { .ok = std::nullopt, .error = "Invalid parameter name: " + update.paramName };
    }

    std::string boardType = board->isTcm() ? "TCM" : "PM";

    double physicalValue = std::stod(update.value);
    int64_t electronicValue = board->calculateElectronic(update.paramName, physicalValue);

    sql::UpdateModel query;
    query.update(db_fit::tables::ConfigurationParameters::TableName).set(db_fit::tables::ConfigurationParameters::ParameterValue.name, std::to_string(electronicValue)).where(sql::column(db_fit::tables::ConfigurationParameters::ConfigurationName.name) == update.configName && sql::column(db_fit::tables::ConfigurationParameters::BoardType.name) == boardType && sql::column(db_fit::tables::ConfigurationParameters::BoardName.name) == update.boardName && sql::column(db_fit::tables::ConfigurationParameters::ParameterName.name) == update.paramName);
    return { .ok = query.str(), .error = std::nullopt };
}

Result<std::string, std::string> ConfigurationDatabaseBroker::constructSelectParameters(std::string_view line)
{
    //[CONFIGURATION NAME / *],[BOARD NAME / *],[PARAMETER NAME / *] ( ,[STARTING DATE] )
    auto result = string_utils::Splitter::getAll(line, ',');
    if (result.isOk() == false) {
        return { .ok = std::nullopt, .error = result.error };
    }
    if (result.ok->size() != SelectParameter::ExpectedSize) {
        return { .ok = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.ok->size()) };
    }
    SelectParameter select(result.ok.value());
    sql::SelectModel query;
    query.select("*").from(db_fit::tables::ConfigurationParameters::TableName);
    if (select.configName != "*") {
        query.where(sql::column(db_fit::tables::ConfigurationParameters::ConfigurationName.name) == select.configName);
    }
    if (select.boardName != "*") {
        query.where(sql::column(db_fit::tables::ConfigurationParameters::BoardName.name) == select.boardName);
    }
    if (select.paramName != "*") {
        query.where(sql::column(db_fit::tables::ConfigurationParameters::ParameterName.name) == select.paramName);
    }
    return { .ok = query.str(), .error = std::nullopt };
}

Result<std::string, std::string> ConfigurationDatabaseBroker::constructSelectVersionsParameters(std::string_view line)
{
    auto result = string_utils::Splitter::getAll(line, ',');
    if (result.isOk() == false) {
        return { .ok = std::nullopt, .error = result.error };
    }
    if (result.ok->size() != SelectParameterVersion::ExpectedSize) {
        return { .ok = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.ok->size()) };
    }
    SelectParameterVersion select(result.ok.value());
    sql::SelectModel query;

    query.select("*").from(db_fit::tables::ConfigurationParameters::TableName);
    if (select.configName != "*") {
        query.where(sql::column(db_fit::tables::ConfigurationParameters::ConfigurationName.name) == select.configName);
    }
    if (select.boardName != "*") {
        query.where(sql::column(db_fit::tables::ConfigurationParameters::BoardName.name) == select.boardName);
    }
    if (select.paramName != "*") {
        query.where(sql::column(db_fit::tables::ConfigurationParameters::ParameterName.name) == select.paramName);
    }
    std::string queryStr = query.str();
    queryStr.insert(queryStr.find("*"), string_utils::concatenate(" ", db_fit::tables::ConfigurationParameters::TableName, "."));
    queryStr.insert(queryStr.find("*") + 1, string_utils::concatenate(",", VersionColumns));
    std::string version = versions(select.startDate);
    size_t versionPos = queryStr.find("where");
    if (versionPos != std::string::npos) {
        queryStr.insert(versionPos - 1, version);
    } else {
        queryStr.append(version);
    }
    return { .ok = std::move(queryStr), .error = std::nullopt };
}

Result<std::string, std::string> ConfigurationDatabaseBroker::constructSelectConfiguration(std::string_view line)
{
    auto result = string_utils::Splitter::getAll(line, ',');
    if (result.isOk() == false) {
        return { .ok = std::nullopt, .error = result.error };
    }
    if (result.ok->size() != SelectConfiguration::ExpectedSize) {
        return { .ok = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.ok->size()) };
    }
    SelectConfiguration select(result.ok.value());

    sql::SelectModel query;
    query.select("*").from(db_fit::tables::Configurations::TableName);
    if (select.configName != "*") {
        query.where(sql::column(db_fit::tables::ConfigurationParameters::ConfigurationName.name) == select.configName);
    }
    if (select.author != "*") {
        query.where(sql::column(db_fit::tables::Configurations::Author.name) == select.author);
    }

    return { .ok = query.str(), .error = std::nullopt };
}

Result<std::string, std::string> ConfigurationDatabaseBroker::constructDeleteParameter(std::string_view line)
{
    auto result = string_utils::Splitter::getAll(line, ',');
    if (result.isOk() == false) {
        return { .ok = std::nullopt, .error = result.error };
    }
    if (result.ok->size() != DeleteParameter::ExpectedSize) {
        return { .ok = std::nullopt, .error = "Unexpected number of tokens: " + std::to_string(result.ok->size()) };
    }
    DeleteParameter deletion(result.ok.value());

    sql::DeleteModel query;
    query.from(db_fit::tables::ConfigurationParameters::TableName);
    query.where(sql::column(db_fit::tables::ConfigurationParameters::ConfigurationName.name) == deletion.configName);
    query.where(sql::column(db_fit::tables::ConfigurationParameters::BoardName.name) == deletion.boardName);
    query.where(sql::column(db_fit::tables::ConfigurationParameters::ParameterName.name) == deletion.paramName);
    return { .ok = query.str(), .error = std::nullopt };
}

/*
    =   =   =   Executors   =   =   =
*/

Result<std::string, std::string> ConfigurationDatabaseBroker::executeSelectParameters(const std::string& query)
{
    std::string rows;
    bool status = false;
    auto results = DatabaseInterface::executeQuery(query, status);
    auto toPhysical = [this](const std::string& boardName, const std::string& parameterName, int64_t electronicValue) {
        double physicalValue = m_Boards[boardName]->calculatePhysical(parameterName, electronicValue);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(3) << physicalValue;
        return ss.str();
    };

    if (!status) {
        return { .ok = std::nullopt, .error = "SELECT query failed: " + query };
    }

    if (results.empty()) {
        return { .ok = rows, .error = std::nullopt };
    }

    for (auto& row : results) {
        std::string boardName = row[db_fit::tables::ConfigurationParameters::BoardName.idx]->getString();
        std::string parameterName = row[db_fit::tables::ConfigurationParameters::ParameterName.idx]->getString();
        std::string value = toPhysical(boardName, parameterName, row[db_fit::tables::ConfigurationParameters::ParameterValue.idx]->getDouble());

        rows += row[db_fit::tables::ConfigurationParameters::ConfigurationName.idx]->getString() + ",";
        rows += boardName + ",";
        rows += parameterName + ",";
        rows += value;
        rows += "\n";
    }
    return { .ok = rows, .error = std::nullopt };
}

Result<std::string, std::string> ConfigurationDatabaseBroker::executeSelectVerionsParameters(const std::string& query)
{
    std::string rows;
    bool status = false;
    auto results = DatabaseInterface::executeQuery(query, status);
    auto toPhysical = [this](const std::string& boardName, const std::string& parameterName, int64_t electronicValue) {
        double physicalValue = m_Boards[boardName]->calculatePhysical(parameterName, electronicValue);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(3) << physicalValue;
        return ss.str();
    };

    if (!status) {
        return { .ok = std::nullopt, .error = "SELECT query failed: " + query };
    }

    if (results.empty()) {
        return { .ok = rows, .error = std::nullopt };
    }

    for (auto& row : results) {
        std::string boardName = row[db_fit::tables::ConfigurationParameters::BoardName.idx]->getString();
        std::string parameterName = row[db_fit::tables::ConfigurationParameters::ParameterName.idx]->getString();
        std::string value = toPhysical(boardName, parameterName, row[db_fit::tables::ConfigurationParameters::ParameterValue.idx]->getDouble());

        rows += row[db_fit::tables::ConfigurationParameters::ConfigurationName.idx]->getString() + ",";
        rows += boardName + ",";
        rows += parameterName + ",";
        rows += value + ",";
        rows += (row[5] != nullptr) ? (row[5]->getString() + ",") : "NULL,";
        rows += (row[6] != nullptr) ? (row[6]->getString() + ",") : "NULL,";
        rows += (row[7] != nullptr) ? (row[7]->getString() + ",") : "NULL,";
        rows += "\n";
    }

    return { .ok = rows, .error = std::nullopt };
}

Result<std::string, std::string> ConfigurationDatabaseBroker::executeUpdate(const std::string& query)
{
    std::string errorMessage;
    DatabaseInterface::executeUpdate(query, errorMessage);
    if (errorMessage.empty() == false) {
        return { .ok = std::nullopt, .error = errorMessage };
    }
    return { .ok = std::nullopt, .error = std::nullopt };
}

Result<std::string, std::string> ConfigurationDatabaseBroker::executeSelectConfiguration(const std::string& query)
{
    std::string rows;
    bool status = false;
    auto results = DatabaseInterface::executeQuery(query, status);

    if (!status) {
        return { .ok = std::nullopt, .error = "SELECT query failed: " + query };
    }

    if (results.empty()) {
        return { .ok = rows, .error = std::nullopt };
    }

    for (auto& row : results) {
        rows += row[db_fit::tables::Configurations::ConfigurationName.idx]->getString() + ",";
        rows += row[db_fit::tables::Configurations::Author.idx]->getString() + ",";
        rows += row[db_fit::tables::Configurations::Date.idx]->getString() + ",";
        rows += (row[db_fit::tables::Configurations::Comment.idx] != nullptr) ? row[db_fit::tables::Configurations::Comment.idx]->getString() : "NULL";
        rows += "\n";
    }
    return { .ok = rows, .error = std::nullopt };
}
