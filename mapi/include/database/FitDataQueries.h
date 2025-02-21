#pragma once

#include <string>

namespace db_fit
{
namespace queries
{
std::string selectParameters(std::string boardType);
std::string selectConnectedDevices();
std::string selectEnvironment();
std::string selectPmHistograms();
} // namespace queries
} // namespace db_fit
