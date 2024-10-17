#include"BasicRequestHandler.h"
#include"Fred/Mapi/indefinitemapi.h"
#include"services/Configurations.h"
#include<string_view>
#include<vector>
#include<functional>

class ResetFEE: public Configurations::BoardConfigurations, public IndefiniteMapi
{
public:
    ResetFEE(std::shared_ptr<Board> TCM, std::vector<std::shared_ptr<Board>> pms): BoardConfigurations(TCM) 
    {
        for(auto& pm: pms){
            m_PMs.emplace_back(pm);
        }
    }
    
    
    SwtSequence switchGBTErrorReports(bool);
    SwtSequence setRestSystem();
    SwtSequence setResetFinished();

    BasicRequestHandler::ParsedResponse applyResetFEE();
    BasicRequestHandler::ParsedResponse checkPMLinks();

    SwtSequence maskPMLink(uint32_t idx, bool mask);

    void processExecution() override;

private:
    std::chrono::milliseconds m_sleepAfterReset{2000};
    std::vector<BasicRequestHandler> m_PMs;
};