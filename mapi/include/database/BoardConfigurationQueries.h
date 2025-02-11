#pragma once
#include"DatabaseViews.h"
#include"sql.h"

namespace db_fit
{
namespace queries
{
std::string selectBoardConfiguration(const std::string& configuration, const std::string& board);
}
}