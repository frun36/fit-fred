#pragma once

#include <list>
#include <unordered_map>
#include <chrono>
#include <string_view>

#include "Fred/Mapi/mapi.h"
#include "Fred/Mapi/indefinitemapi.h"
#include "BoardCommunicationHandler.h"
#include "BasicFitIndefiniteMapi.h"
#include "gbtInterfaceUtils.h"

class BoardStatus : public BasicFitIndefiniteMapi, gbt::GbtRateMonitor
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

    bool m_saveGbtErrorToFile;
    std::shared_ptr<gbt::GbtErrorType> m_gbtError;
};