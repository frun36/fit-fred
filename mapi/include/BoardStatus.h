#pragma once

#include<list>
#include<unordered_map>
#include<chrono>
#include<string_view>

#include "Fred/Mapi/mapi.h"
#include "Fred/Mapi/indefinitemapi.h"
#include"BasicRequestHandler.h"
#include"GBT.h"

class BoardStatus: public IndefiniteMapi, BasicRequestHandler, gbt_rate::GbtRateMonitor{
public:
    BoardStatus(std::shared_ptr<Board> board, std::list<std::string> toRefresh);
    void processExecution() final;
    
private:
    static constexpr std::string_view ActualSystemClock{"BOARD_STATUS_ACTUAL_CLOCK_SOURCE"};
    void updateEnvironment();
    WinCCResponse checkGBTErrors();

    SwtSequence m_request;
};