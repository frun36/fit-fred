#include"database/FitDataQueries.h"

namespace db_fit
{
namespace queries
{
    std::string selectParameters(std::string boardType)
    {
        sql::SelectModel query;
        query.select("*").from(db_fit::tabels::Parameters::TableName).where(sql::column(db_fit::tabels::Parameters::BoardType.name) == boardType);
        return query.str();
    }
    
    std::string selectConnectedDevices()
    {
        sql::SelectModel query;
        query.select("*").from(db_fit::tabels::ConnectedDevices::TableName);
        return query.str();
    }

    std::string selectEnvironment()
    {
        sql::SelectModel query;
        query.select("*").from(db_fit::tabels::Environment::TableName);
        return query.str();
    }

}    
}