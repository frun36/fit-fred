#include "services/FredManager.h"
#include<unistd.h>

FredManager::FredManager(std::shared_ptr<Board> TCM, const std::string& configSrv, const std::string& resetSystemSrv, const std::string& resetErrorsSrv, const std::list<std::string>& looping) : m_TCM(TCM),
m_configurationService(configSrv), m_resetErrorsService(resetErrorsSrv), m_resetSystemService(resetSystemSrv)
{
    for(auto& srv: looping){
        m_startLooping.emplace_back(srv, StartService);
        m_stopLooping.emplace_back(srv, StopService);
    }
}

std::string FredManager::processInputMessage(string msg)
{   
    std::istringstream stream(msg);
    std::string cmd;
    std::optional<std::string> config{std::nullopt};
    string_utils::Splitter split(msg,',');
    try{
        cmd = split.getNext();
        if(cmd == StartService && !split.reachedEnd()){
            config = split.getNext();
        }
    }
    catch(std::exception& e){
        publishError(string_utils::concatenate("Failed to parse command: ", msg,"; Exception: ", e.what()));
        return{};
    }

    if(cmd == StartService){
        startServices(config);
    } else if(cmd == StopService){
        stopServices();
    }

    noRpcRequest = true;
    return {};
}
void FredManager::startServices(std::optional<std::string> config)
{
    newMapiGroupRequest({{m_resetSystemService, ReinitializeSpiMaks}});

    usleep(DelayAfterReinitializeSpiMask);

    if(config.has_value()){
        newMapiGroupRequest({{m_configurationService,config.value()}});
        usleep(DelayAfterConfiguration);
        while(m_TCM->getEnvironment("TCM_CONFIG")==1){
            usleep(1'000);
        }
        newMapiGroupRequest({{m_resetErrorsService,""}});
    }
    
    newMapiGroupRequest(m_startLooping);
}

void FredManager::stopServices()
{
    newMapiGroupRequest(m_stopLooping);
}

std::string FredManager::processOutputMessage(string msg)
{
    return {};
}