#include "database/FitDataQueries.h"
#include "database/DatabaseViews.h"
#include "database/sql.h"

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

std::string selectPmHistograms()
{
    sql::SelectModel query;
    query.select(
             tabels::PmChannelHistograms::Name.name,
             tabels::PmChannelHistogramStructure::BaseAddress.name,
             tabels::PmChannelHistogramStructure::RegBlockSize.name,
             tabels::PmChannelHistogramStructure::StartBin.name,
             tabels::PmChannelHistogramStructure::BinsPerRegister.name,
             tabels::PmChannelHistogramStructure::BinDirection.name)
        .from(tabels::PmChannelHistograms::TableName)
        .join(tabels::PmChannelHistogramStructure::TableName)
        .on(sql::column(tabels::PmChannelHistograms::Id.name) == sql::column(tabels::PmChannelHistogramStructure::PmHistogramId.name));
    return query.str();
}
} // namespace queries
} // namespace db_fit
