#pragma once

#include "Fred/Mapi/mapigroup.h"
#include "board/Board.h"
#include <cstring>
#include <list>
#include <memory>

class FredManager : public Mapigroup
{
   public:
    FredManager(std::shared_ptr<Board> TCM, const std::string& configSrv, const std::string& resetSystemSrv,
                const std::string& resetErrorsSrv, const std::list<std::string>& looping, const std::list<std::string>& configs);
    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override;

   private:
    std::shared_ptr<Board> m_TCM;
    void startServices(std::optional<std::string> config);
    void stopServices();

    const std::string StartService{ "START" };
    const std::string StopService{ "STOP" };
    const std::string ReinitializeSpiMask{ "REINITIALIZE_SPI_MASK" };

    const useconds_t DelayAfterReinitializeSpiMask{ 500'000 };
    const useconds_t DelayAfterConfiguration{ 250'000 };

    std::vector<std::pair<std::string, std::string>> m_startLooping;
    std::vector<std::pair<std::string, std::string>> m_stopLooping;
    const std::list<std::string> m_configs;
    const std::string m_configurationService;
    const std::string m_resetSystemService;
    const std::string m_resetErrorsService;
};
