#pragma once

#include<list>
#include<unordered_map>
#include<chrono>
#include<string_view>

#include "Fred/Mapi/mapi.h"
#include "Fred/Mapi/indefinitemapi.h"
#include"BasicRequestHandler.h"

class BoardStatus: public IndefiniteMapi, BasicRequestHandler{
public:
    BoardStatus(std::shared_ptr<Board> board, std::list<std::string> toRefresh);
    void processExecution() final;
    
private:
    static constexpr const char* ACTUAL_SYSTEM_CLOCK_NAME = "BOARD_STATUS_ACTUAL_CLOCK_SOURCE";
    static constexpr const char* WORDS_COUNT_NAME = "GBT_WORDS_COUNT";
    static constexpr const char* EVENTS_COUNT_NAME = "GBT_EVENTS_COUNT";
    static constexpr const char* GBT_ERROR_REPORT_EMPTY = "GBT_ERROR_REPORT_EMPTY";
    
    static constexpr const char* GBT_WORD_RATE_NAME = "GBT_WORD_RATE";
    static constexpr const char* GBT_EVENT_RATE_NAME = "GBT_EVENT_RATE";
    static constexpr const char* GBT_ERROR_NAME = "GBT_ERROR";

    static constexpr const char* EXTERNAL_CLOCK_VNAME = "LHC_CLOCK";
    static constexpr const char* INTERNAL_CLOCL_VNAME = "INTERNAL_CLOCK";
    static constexpr const char* SYSTEM_CLOCK_VNAME = "SYSTEM_CLOCK";
    static constexpr const char* TDC_VNAME = "TDC";

    static constexpr bool INTERNAL_CLOCK = false;
    static constexpr bool EXTERNAL_CLOCK = true;

    void updateEnvironment();
    void calculateGBTRate(WinCCResponse& response);
    void checkGBTErrorReport(WinCCResponse& response);
    void readGBTErrorFIFO(WinCCResponse& response);

    struct GBTRate{
        uint32_t wordsCount;
        uint32_t eventsCount;
    } m_gbtRate;

    SwtSequence m_request;

    std::chrono::milliseconds m_pomTimeInterval;
    std::chrono::time_point<std::chrono::steady_clock> m_lastTimePoint;
    std::chrono::time_point<std::chrono::steady_clock> m_currTimePoint;
};