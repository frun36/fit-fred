#include "../BasicRequestHandler.h"
#include "Fred/Mapi/indefinitemapi.h"
#include "Configurations.h"
#include "Alfred/print.h"
#include "../utils.h"
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
    BasicRequestHandler::ParsedResponse testPMLinks();
    BasicRequestHandler::ParsedResponse applyGbtConfiguration();
    BasicRequestHandler::ParsedResponse applyGbtConfigurationToBoard(BasicRequestHandler& boardHandler);
    BasicRequestHandler::ParsedResponse applyTriggersSign();

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

    template <typename T>
    std::string writeRequest(std::string_view param, T value)
    {
        return string_utils::concatenate(param, ",WRITE,", std::to_string(value));
    }

    std::string readRequest(std::string_view param)
    {
        return string_utils::concatenate(param, "READ");
    }

    uint32_t getEnvBoardId(std::shared_ptr<Board> board);

    static const BasicRequestHandler::ParsedResponse EmptyResponse;
    static constexpr std::string_view EnforceDefGbtConfig{ "ENFORCE DEFAULT GBT CONFIG" };
    static constexpr std::string_view ForceLocalClock{ "FORCE LOCAL CLOCK" };

    bool m_enforceDefGbtConfig{ false };
    bool m_forceLocalClock{ false };

    std::chrono::milliseconds m_sleepAfterReset{ 2000 };
    std::vector<BasicRequestHandler> m_PMs;
};