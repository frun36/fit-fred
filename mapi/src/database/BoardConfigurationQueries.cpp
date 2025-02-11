#include "database/BoardConfigurationQueries.h"

namespace db_fit
{
namespace queries
{
std::string selectBoardConfiguration(const std::string& configuration, const std::string& board)
{
    sql::SelectModel query;
    query
        .select(db_fit::tables::ConfigurationParameters::ParameterName.name, db_fit::tables::ConfigurationParameters::ParameterValue.name)
        .from(db_fit::tables::ConfigurationParameters::TableName)
        .where(sql::column(db_fit::tables::ConfigurationParameters::ConfigurationName.name) == configuration && sql::column(db_fit::tables::ConfigurationParameters::BoardName.name) == board);
    return query.str();
}
} // namespace queries
} // namespace db_fit
