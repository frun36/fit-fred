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
    using  namespace db_fit::tabels;
    sql::SelectModel query;
    query.select(
             PmChannelHistograms::Name.name,
             PmChannelHistogramStructure::BaseAddress.name,
             PmChannelHistogramStructure::RegBlockSize.name,
             PmChannelHistogramStructure::StartBin.name,
             PmChannelHistogramStructure::BinsPerRegister.name,
             PmChannelHistogramStructure::BinDirection.name)
        .from(tabels::PmChannelHistograms::TableName)
        .join(tabels::PmChannelHistogramStructure::TableName)
        .on(sql::column(fullColumnName(PmChannelHistograms::TableName,PmChannelHistograms::Id.name)) == 
            sql::column(fullColumnName(PmChannelHistogramStructure::TableName,PmChannelHistogramStructure::PmHistogramId.name)));
    return query.str();
}
} // namespace queries
} // namespace db_fit
