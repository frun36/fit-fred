#pragma once

#include <algorithm>
#include <functional>
#include <vector>
#include "Database/databaseinterface.h"

namespace db_fit
{
namespace queries
{
template <typename RowType, typename... Args>
struct Query {

    RowType parseRow(const std::vector<MultiBase*>& row);
    std::vector<RowType> parse(const std::vector<std::vector<MultiBase*>>& queryResult)
    {
        std::vector<RowType> parsed;
        std::transform(queryResult.begin(), queryResult.end(), std::back_inserter(parsed),
                       [](const std::vector<MultiBase*>& query) -> RowType { return { query }; });
        return parsed;
    }

   private:
    std::function<std::string(const Args&...)> parser;
};

} // namespace queries

} // namespace db_fit
