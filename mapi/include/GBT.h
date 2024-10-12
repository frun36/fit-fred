#pragma once

#include<cstdint>
#include<string_view>
#include<chrono>
#include<memory>
#include"WinCCResponse.h"

struct GBTWord
{
    static constexpr uint32_t WordSize = 5;
    uint16_t buffer[WordSize];
};

namespace gbt_error
{

namespace parameters
{
constexpr std::string_view Fifo{"GBT_ERROR_REPORT"};
constexpr std::string_view BCSyncLost{"GBT_ERR_BC_SYNC_LOST"};
constexpr std::string_view PMEarlyHeader{"GBT_ERR_PM_EARLY_HEADER"};
constexpr std::string_view FifoOverload{"GBT_ERR_FIFO_OVERLOAD"};
constexpr std::string_view FifoEmpty{"GBT_FIFO_EMPTY_ERROR_REPORT"};
}

namespace constants
{
constexpr uint32_t fifoSize = 36;
constexpr double fifoEmpty = 0;
}

struct GbtErrorType
{
    virtual WinCCResponse createWinCCResponse() = 0;
};

struct BCSyncLost: public GbtErrorType
{
    BCSyncLost(const std::array<uint32_t, constants::fifoSize>& fifoData);
    [[nodiscard]] WinCCResponse createWinCCResponse() final;
    static constexpr uint32_t getErrorCode() {return  0xEEEE000A; }
    
    struct Data{
        uint32_t errorCode;
        struct {
            GBTWord data;
            uint16_t counter : 12,
            isData  :  4;
        } words[10];                    
        uint16_t BC_CRU; 
        uint16_t BC_Board;   
        uint32_t orbitBoard;         
        uint32_t orbitCRU;         
        uint32_t reservedSpace[2]; 
    } data;
};

struct PMEarlyHeader: public GbtErrorType
{
    PMEarlyHeader(const std::array<uint32_t, constants::fifoSize>& fifoData);
    [[nodiscard]] WinCCResponse createWinCCResponse();
    static constexpr uint32_t getErrorCode() {return 0xEEEE0009;}
    
    struct Data{
        uint32_t errorCode;
        GBTWord words[14];
    } data;
};

struct FifoOverload: public GbtErrorType
{
    FifoOverload(const std::array<uint32_t, constants::fifoSize>& fifoData);
    [[nodiscard]] WinCCResponse createWinCCResponse();
    static constexpr uint32_t getErrorCode() {return 0xEEEE0008;}

    struct Data{
            GBTWord w[13];              
            uint16_t reservedSpace0;    
            uint16_t rdRate;
            uint16_t wrRate;     
            uint32_t _reservedSpace1; 
    }  data;
};

[[nodiscard]] std::shared_ptr<GbtErrorType> parseFifoData(const std::array<uint32_t, constants::fifoSize>& fifoData);

}

namespace gbt_rate
{
namespace parameters
{
    constexpr std::string_view WordsRate{"GBT_WORDS_RATE"};
    constexpr std::string_view EventsRate{"GBT_EVENTS_RATE"};
    constexpr std::string_view WordsCount{"GBT_WORDS_COUNT"};
    constexpr std::string_view EventsCount{"GBT_EVENTS_COUNT"};
}

namespace constants
{

}

class GbtRate
{
public:
    GbtRate(): oldCount(0) {}
    double updateRate(uint32_t newCount, std::chrono::milliseconds timeInterval);
private:
    uint32_t oldCount;
};

class GbtRateMonitor
{
public:

    inline void updateTimePoint()
    {
        m_currTimePoint = std::chrono::steady_clock::now();
        m_timeInterval = std::chrono::duration_cast<std::chrono::milliseconds>(m_currTimePoint-m_lastTimePoint);
        m_lastTimePoint = m_currTimePoint;
    }

    WinCCResponse updateRates(uint32_t wordsCount, uint32_t eventsCount);

private:
    std::chrono::milliseconds m_timeInterval;
    std::chrono::time_point<std::chrono::steady_clock> m_lastTimePoint;
    std::chrono::time_point<std::chrono::steady_clock> m_currTimePoint;

    
    GbtRate m_wordsRate;
    GbtRate m_eventsRate;
};

} // gbt_rate


