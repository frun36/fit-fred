#pragma once

#include<list>
#include<unordered_map>
#include<chrono>
#include<string_view>

#include "Fred/Mapi/mapi.h"
#include "Fred/Mapi/indefinitemapi.h"
#include"BasicRequestHandler.h"
#include"GBT.h"

class BoardStatus: public IndefiniteMapi, BasicRequestHandler, gbt_rate::GBTRateMonitor{
public:
    BoardStatus(std::shared_ptr<Board> board, std::list<std::string> toRefresh);
    void processExecution() final;
    
private:
    static constexpr const char* ACTUAL_SYSTEM_CLOCK_NAME = "BOARD_STATUS_ACTUAL_CLOCK_SOURCE";

    static constexpr const char* EXTERNAL_CLOCK_VNAME = "LHC_CLOCK";
    static constexpr const char* INTERNAL_CLOCL_VNAME = "INTERNAL_CLOCK";
    static constexpr const char* SYSTEM_CLOCK_VNAME = "SYSTEM_CLOCK";
    static constexpr const char* TDC_VNAME = "TDC";

    static constexpr bool INTERNAL_CLOCK = false;
    static constexpr bool EXTERNAL_CLOCK = true;

    void updateEnvironment();
    WinCCResponse checkGBTErrors();

    SwtSequence m_request;
};