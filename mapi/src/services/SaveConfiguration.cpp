#include "services/SaveConfiguration.h"
#include "database/sql.h"
#include "Alfred/print.h"
#include "utils.h"
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

std::string SaveConfiguration::makeEntry(std::string_view line)
{
    if(line.size() == 0){
        throw std::runtime_error("Empty entry");
    }
    
    size_t start = 0;
    size_t stop = line.find(',', start+1);

    if(stop == start || stop == (start + 1) || stop == std::string::npos){
        throw std::runtime_error(string_utils::concatenate("Unexpected entry: ", line));
    }
    std::string configurationName{ line.substr(start, stop) };   
    if(m_knownConfigs.find(configurationName) == m_knownConfigs.end()){
        throw std::runtime_error(string_utils::concatenate("Unknown configuration: ", configurationName));
    }

    start = stop+1;
    stop = line.find(',', start);
    if(stop == start || stop == (start + 1) || stop == std::string::npos){
        throw std::runtime_error("Unexpected entry: " + std::string(line));
    }
    std::string boardName {line.substr(start, stop-start)};
    auto boardItr = m_Boards.find(boardName);
    if(boardItr == m_Boards.end()){
        throw std::runtime_error(string_utils::concatenate("Unknown board: ", boardName));
    }
    auto board = boardItr->second;
    Print::PrintData(boardName);
    std::string_view boradType = board->isTcm() ? "TCM" : "PM";

    start = stop+1;
    stop = line.find(',', start);
    if(stop == start || stop == (start + 1) || stop == std::string::npos){
        throw std::runtime_error("Unexpected entry: " + std::string(line));
    }
    std::string parameterName{ line.substr(start, stop-start) };
    Print::PrintData("|" + parameterName + "|");
    if(!board->doesExist(parameterName)){
        throw std::runtime_error(string_utils::concatenate("Unknown parameter: ", parameterName));
    }

    start = stop+1;
    double physcialValue = std::stod(line.substr(start).data());
    int64_t electronicValue = board->calculateElectronic(parameterName.data(), physcialValue);

    
    sql::InsertModel query;
    query.insert(db_tables::ConfigurationParameters::ConfigurationName.name, configurationName)
                (db_tables::ConfigurationParameters::BoardName.name, boardName)
                (db_tables::ConfigurationParameters::BoardType.name, boradType.data())
                (db_tables::ConfigurationParameters::ParameterName.name, parameterName)
                (db_tables::ConfigurationParameters::ParameterValue.name, std::to_string(electronicValue)).into(db_tables::ConfigurationParameters::TableName);

    return query.str();
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
    std::string query;

    size_t lineBeg = 0;
    size_t lineEnd = std::numeric_limits<size_t>::max();
    size_t lineNumber = 0;
    std::string errorMessage;
    bool parsed =  true;
    
    do {
        lineBeg = lineEnd+1;
        lineEnd = request.find('\n',lineBeg);
        if(lineEnd - lineBeg == 0){
            Print::PrintWarning(name, "Line " + std::to_string(lineNumber) + " is empty. Skipping");
            continue;
        }
        try{
            query.append(makeEntry(std::string_view(&request[lineBeg], lineEnd-lineBeg)));
        }
        catch(std::runtime_error& err){
            errorMessage = err.what();
            parsed = false;
            break;
        }
        catch(std::exception& err){
            errorMessage = err.what();
            parsed = false;
            break;
        }
        lineNumber++;
    }while(lineEnd != std::string::npos);

    if(!parsed){
        std::string message = "Parsing request failed at line: " + std::to_string(lineNumber) + ". Message: " + errorMessage;
        Print::PrintError(name, message);
        publishError(message);
        return;
    }

    DatabaseInterface::executeUpdate(query, errorMessage);
    if(errorMessage.empty() == false){
        Print::PrintError(name, errorMessage);
        publishError(errorMessage);
    } else {
    publishAnswer("Updated database");
    }
}