#pragma once
#include"DatabaseViews.h"
#include "sql.h"
namespace db_fit
{
namespace queries
{
    std::string selectParameters(std::string boardType);
    std::string selectConnectedDevices();
    std::string selectEnvironment();
}    
}