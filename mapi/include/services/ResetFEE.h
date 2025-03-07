#include "services/templates/BasicFitIndefiniteMapi.h"

class ResetFEE : public BasicFitIndefiniteMapi
{
   public:
    ResetFEE(std::shared_ptr<Board> TCM, std::vector<std::shared_ptr<Board>> pms) : m_TCM(TCM)
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
    std::string seqCntUpdateRate(uint8_t updateRate);

    BoardCommunicationHandler::ParsedResponse applyResetFEE();
    BoardCommunicationHandler::ParsedResponse updatePmSpiMask();
    BoardCommunicationHandler::ParsedResponse applyGbtConfiguration();
    BoardCommunicationHandler::ParsedResponse applyGbtConfigurationToBoard(BoardCommunicationHandler& boardHandler);
    BoardCommunicationHandler::ParsedResponse applyTriggersSign();

    uint32_t getEnvBoardId(std::shared_ptr<Board> board);

    static constexpr std::string_view EnforceDefGbtConfig{ "ENFORCE_DEFAULT_GBT_CONFIG" };
    static constexpr std::string_view ForceLocalClock{ "FORCE LOCAL CLOCK" };
    static constexpr std::string_view ReinitializeSpiMask{"REINITIALIZE SPI MASK"};

    bool m_enforceDefGbtConfig{ false };
    bool m_forceLocalClock{ false };

    bool m_initialized{ false };
    uint32_t m_channelMaskATmp{ 0 };
    uint32_t m_channelMaskCTmp{ 0 };

    std::chrono::milliseconds m_sleepAfterReset{ 2000 };
    std::vector<BoardCommunicationHandler> m_PMs;
    BoardCommunicationHandler m_TCM;
};
