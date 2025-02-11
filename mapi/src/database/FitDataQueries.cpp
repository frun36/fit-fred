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
    query.select("*").from(db_fit::tables::Parameters::TableName).where(sql::column(db_fit::tables::Parameters::BoardType.name) == boardType);
    return query.str();
}

std::string selectConnectedDevices()
{
    sql::SelectModel query;
    query.select("*").from(db_fit::tables::ConnectedDevices::TableName);
    return query.str();
}

std::string selectEnvironment()
{
    sql::SelectModel query;
    query.select("*").from(db_fit::tables::Environment::TableName);
    return query.str();
}

std::string selectPmHistograms()
{
    sql::SelectModel query;
    query.select(
             tables::PmChannelHistograms::Name.name,
             tables::PmChannelHistogramStructure::BaseAddress.name,
             tables::PmChannelHistogramStructure::RegBlockSize.name,
             tables::PmChannelHistogramStructure::StartBin.name,
             tables::PmChannelHistogramStructure::BinsPerRegister.name,
             tables::PmChannelHistogramStructure::BinDirection.name)
        .from(tables::PmChannelHistograms::TableName)
        .join(tables::PmChannelHistogramStructure::TableName)
        .on(sql::column(tables::PmChannelHistograms::Id.name) == sql::column(tables::PmChannelHistogramStructure::PmHistogramId.name));
    return query.str();
}
} // namespace queries
} // namespace db_fit
