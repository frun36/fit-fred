#include "BasicRequestHandler.h"
#include "Fred/Mapi/indefinitemapi.h"
#include "services/Configurations.h"
#include <string_view>
#include <vector>
#include <functional>

class ResetFEE : public Configurations::BoardConfigurations, public IndefiniteMapi
{
   public:
    ResetFEE(std::shared_ptr<Board> TCM, std::vector<std::shared_ptr<Board>> pms) : BoardConfigurations(TCM)
    {
        for (auto& pm : pms) {
            m_PMs.emplace_back(pm);
        }
    }

    std::string seqSwitchGBTErrorReports(bool);
    std::string seqSetResetSystem();
    std::string seqSetResetFinished();
    std::string seqSetBoardId(std::shared_ptr<Board> board);
    std::string seqSetSystemId();
    std::string seqMaskPMLink(uint32_t idx, bool mask);

    BasicRequestHandler::ParsedResponse applyResetFEE();
    BasicRequestHandler::ParsedResponse checkPMLinks();

    BasicRequestHandler::ParsedResponse processSequence(BasicRequestHandler& handler, std::string request)
    {
        std::string seq;
        try {
            seq = handler.processMessageFromWinCC(request).getSequence();
        } catch (...) {
            return { WinCCResponse(), { { "REQUEST_TO_ALF", "Request failed: " + request } } };
        }
        return handler.processMessageFromALF(executeAlfSequence(seq));
    }

    void processExecution() override;

   private:
    std::chrono::milliseconds m_sleepAfterReset{ 2000 };
    std::vector<BasicRequestHandler> m_PMs;
};