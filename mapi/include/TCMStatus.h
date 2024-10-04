#pragma once

#include<list>
#include<unordered_map>
#include<chrono>
#include<string_view>
#include "Fred/Mapi/mapi.h"
#include"BasicRequestHandler.h"

class TCMStatus: public Mapi, BasicRequestHandler{
public:
    TCMStatus(std::shared_ptr<Board> board, std::list<std::string> toRefresh);
    std::string processInputMessage(string msg) override;
    std::string processOutputMessage(string msg) override;
    
private:
    constexpr const char* ACTUAL_SYSTEM_CLOCK_NAME = "BOARD_STATUS_ACTUAL_CLOCK_SOURCE";

    constexpr const char* EXTERNAL_CLOCK_VNAME = "LHC_CLOCK";
    constexpr const char* INTERNAL_CLOCL_VNAME = "INTERNAL_CLOCK";
    constexpr const char* SYSTEM_CLOCK_VNAME = "SYSTEM_CLOCK";
    constexpr const char* TDC_VNAME = "TDC";

    constexpr bool INTERNAL_CLOCK = false;
    constexpr bool EXTERNAL_CLOCK = true;

    SwtSequence m_request;

    std::chrono::milliseconds m_pomTimeInterval;
    std::chrono::time_point<std::chrono::steady_clock> m_lastTimePoint;
    std::chrono::time_point<std::chrono::steady_clock> m_currTimePoint;
};