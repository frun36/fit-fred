#pragma once

#include <list>
#include <string_view>

#include "board/BoardCommunicationHandler.h"
#include "services/templates/LoopingFitIndefiniteMapi.h"
#include "utils/gbtInterfaceUtils.h"

class BoardStatus : public LoopingFitIndefiniteMapi, gbt::GbtRateMonitor
{
   public:
    BoardStatus(std::shared_ptr<Board> board, std::list<std::string> toRefresh);
    void processExecution() final;

   private:
    static constexpr std::string_view ActualSystemClock{ "BOARD_STATUS_ACTUAL_CLOCK_SOURCE" };
    void updateEnvironment();

    BoardCommunicationHandler::ParsedResponse checkGbtErrors();

    BoardCommunicationHandler m_boardHandler;
    BoardCommunicationHandler m_gbtFifoHandler;
    SwtSequence m_request;

    std::shared_ptr<gbt::GbtErrorType> m_gbtError;
};
