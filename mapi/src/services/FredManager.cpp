#include "Alfred/print.h"
#include "services/FredManager.h"
#include "utils/utils.h"
#include <sstream>
#include <unistd.h>

FredManager::FredManager(std::shared_ptr<Board> TCM, const std::string& configSrv, const std::string& resetSystemSrv,
                         const std::string& resetErrorsSrv, const std::list<std::string>& looping, const std::list<std::string>& configs)
    : m_TCM(TCM),
      m_configs(configs),
      m_configurationService(configSrv),
      m_resetSystemService(resetSystemSrv),
      m_resetErrorsService(resetErrorsSrv)
{
    for (auto& srv : looping) {
        m_startLooping.emplace_back(srv, StartService);
        m_stopLooping.emplace_back(srv, StopService);
    }
}

std::string FredManager::processInputMessage(string msg)
{
    std::istringstream stream(msg);
    std::string cmd;
    std::optional<std::string> config{ std::nullopt };
    string_utils::Splitter split(msg, ',');
    try {
        cmd = split.getNext();
        if (cmd == StartService && !split.reachedEnd()) {
            config = split.getNext();
        }
    } catch (std::exception& e) {
        publishError(string_utils::concatenate("Failed to parse command: ", msg, "; Exception: ", e.what()));
        return {};
    }

    if (cmd == StartService) {
        startServices(config);
    } else if (cmd == StopService) {
        stopServices();
    }
    publishAnswer("SUCCESS");
    noRpcRequest = true;
    return {};
}

void FredManager::startServices(std::optional<std::string> config)
{
    Print::PrintWarning(name, "Manager requesting " + ReinitializeSpiMask);
    newMapiGroupRequest({ { m_resetSystemService, ReinitializeSpiMask } });
    usleep(10'000);
    while (m_TCM->getEnvironment("RESET_SYSTEM") == 1) {
        usleep(1'000);
    }

    if (config.has_value()) {
        Print::PrintWarning(name, "Manager requesting configuration '" + config.value() + "'");
        newMapiGroupRequest({ { m_configurationService, config.value() } });
        usleep(10'000);
        uint32_t running = 0;
        while (running != m_configs.size()) {
            running = 0;
            for (const auto& config : m_configs) {
                if (m_TCM->getEnvironment(config) == 0) {
                    running += 1;
                }
            }
            usleep(1'000);
        }
        Print::PrintWarning(name, "Manager requesting RESET_ERRORS");
        newMapiGroupRequest({ { m_resetErrorsService, "" } });
    }

    Print::PrintWarning(name, "Manager requesting start of looping services");
    newMapiGroupRequest(m_startLooping);
}

void FredManager::stopServices()
{
    Print::PrintWarning(name, "Manager requesting stop of looping services");
    newMapiGroupRequest(m_stopLooping);
}

std::string FredManager::processOutputMessage(string)
{
    return {};
}
