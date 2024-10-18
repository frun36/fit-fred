#include "BasicRequestHandler.h"
#include "Fred/Mapi/indefinitemapi.h"
#include "services/Configurations.h"
#include "Alfred/print.h"
#include <string_view>
#include <vector>
#include <functional>

class ResetFEE : public BasicRequestHandler, public IndefiniteMapi
{
   public:
    ResetFEE(std::shared_ptr<Board> TCM, std::vector<std::shared_ptr<Board>> pms) : BasicRequestHandler(TCM)
    {
        for (auto& pm : pms) {
            m_PMs.emplace_back(pm);
        }
    }

    void processExecution() override;

   private:
    std::string seqSwitchGBTErrorReports(bool);
    std::string seqSetResetSystem();
    std::string seqSetResetFinished();
    std::string seqSetBoardId(std::shared_ptr<Board> board);
    std::string seqSetSystemId();
    std::string seqMaskPMLink(uint32_t idx, bool mask);

    BasicRequestHandler::ParsedResponse applyResetFEE();
    BasicRequestHandler::ParsedResponse checkPMLinks();
    BasicRequestHandler::ParsedResponse applyGbtConfiguration();
    BasicRequestHandler::ParsedResponse applyGbtConfigurationToBoard(BasicRequestHandler& boardHandler);

    BasicRequestHandler::ParsedResponse processSequence(BasicRequestHandler& handler, std::string request)
    {
        std::string seq;
        try {
            seq = handler.processMessageFromWinCC(request).getSequence();
        } catch (const std::exception& e) {
            Print::PrintVerbose(e.what());
            return { WinCCResponse(), { { handler.getBoard()->getName(), e.what() } } };
        }
        return handler.processMessageFromALF(executeAlfSequence(seq));
    }

    static const BasicRequestHandler::ParsedResponse EmptyResponse;

    std::chrono::milliseconds m_sleepAfterReset{ 2000 };
    std::vector<BasicRequestHandler> m_PMs;
};