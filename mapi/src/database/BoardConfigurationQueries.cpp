#include"database/BoardConfigurationQueries.h"


namespace db_fit
{
namespace queries
{
std::string selectBoardConfiguration(const std::string& configuration, const std::string& board)
{
 sql::SelectModel query;
    query
        .select(db_fit::tabels::ConfigurationParameters::ParameterName.name, db_fit::tabels::ConfigurationParameters::ParameterValue.name)
        .from(db_fit::tabels::ConfigurationParameters::TableName)
        .where(sql::column(db_fit::tabels::ConfigurationParameters::ConfigurationName.name) == configuration && sql::column(db_fit::tabels::ConfigurationParameters::BoardName.name) == board);
    return query.str();
}
}
}