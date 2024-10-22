#include "BasicFitIndefiniteMapi.h"
#include "Configurations.h"
#include "Alfred/print.h"
#include "../utils.h"
#include <string_view>
#include <vector>
#include <functional>

class ResetFEE : public BasicFitIndefiniteMapi
{
   public:
    ResetFEE(std::shared_ptr<Board> TCM, std::vector<std::shared_ptr<Board>> pms) : BasicFitIndefiniteMapi(TCM)
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

    uint32_t getEnvBoardId(std::shared_ptr<Board> board);
    uint32_t prepareSign(double sign)
    {
        return static_cast<uint32_t>(sign) << 7 | (~static_cast<uint32_t>(sign) & 0x7F);
    }

    static constexpr std::string_view EnforceDefGbtConfig{ "ENFORCE_DEFAULT_GBT_CONFIG" };
    static constexpr std::string_view ForceLocalClock{ "FORCE LOCAL CLOCK" };

    bool m_enforceDefGbtConfig{ false };
    bool m_forceLocalClock{ false };

    std::chrono::milliseconds m_sleepAfterReset{ 2000 };
    std::vector<BasicRequestHandler> m_PMs;
};