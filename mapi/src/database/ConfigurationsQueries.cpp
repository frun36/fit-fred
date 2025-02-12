#include "database/ConfigurationsQueries.h"

namespace db_fit
{
namespace queries
{
std::string selectDistinctBoards(const std::string& configuration)
{
    sql::SelectModel query;
    query
        .select(db_fit::tables::ConfigurationParameters::BoardName.name)
        .distinct()
        .from(db_fit::tables::ConfigurationParameters::TableName)
        .where(sql::column(db_fit::tables::ConfigurationParameters::ConfigurationName.name) == configuration);
    return query.str();
}
} // namespace queries
} // namespace db_fit
