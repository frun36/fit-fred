#pragma once

#include <cstdint>
#include <string_view>
#include <chrono>
#include <memory>
#include<array>
#include "communication-utils/WinCCResponse.h"

struct GbtWord {
    static constexpr uint32_t WordSize = 5;
    uint16_t buffer[WordSize];
};

namespace gbt
{

namespace parameters
{
constexpr std::string_view Fifo{ "GBT_ERROR_REPORT" };
constexpr std::string_view BCSyncLost{ "GBT_ERR_BC_SYNC_LOST" };
constexpr std::string_view PmEarlyHeader{ "GBT_ERR_PM_EARLY_HEADER" };
constexpr std::string_view FifoOverload{ "GBT_ERR_FIFO_OVERLOAD" };
constexpr std::string_view FifoEmpty{ "GBT_FIFO_EMPTY_ERROR_REPORT" };

// 0xD8
constexpr std::string_view ResetOrbitSync{ "GBT_RESET_ORBIT_SYNC" };
constexpr std::string_view ResetDataCounters{ "GBT_RESET_DATA_COUNTERS" };
constexpr std::string_view ResetStartEmulation{ "GBT_START_OF_EMULATION" };
constexpr std::string_view ResetRxError{ "GBT_RESET_RX_ERROR" };
constexpr std::string_view Reset{ "GBT_RESET" };
constexpr std::string_view ResetRxPhaseError{ "GBT_RESET_RX_PHASE_ERROR" };
constexpr std::string_view ResetReadoutFsm{ "GBT_RESET_READOUT_FSM" };
constexpr std::string_view FifoReportReset{ "GBT_RESET_ERROR_REPORT_FIFO" };
constexpr std::string_view ForceIdle{ "GBT_FORCE_IDLE" };

constexpr std::string_view BoardId{ "GBT_RDH_FEEID" };
constexpr std::string_view SystemId{ "GBT_RDH_SYSTEM_ID" };
constexpr std::string_view BcIdDelay{ "GBT_BCID_OFFSET" };

constexpr std::string_view WordsRate{ "GBT_WORDS_RATE" };
constexpr std::string_view EventsRate{ "GBT_EVENTS_RATE" };
constexpr std::string_view WordsCount{ "GBT_WORDS_COUNT" };
constexpr std::string_view EventsCount{ "GBT_EVENTS_COUNT" };
} // namespace parameters

namespace constants
{
constexpr size_t FifoSize = 36*sizeof(uint32_t);
constexpr double FifoEmpty = 1;
} // namespace constants

struct GbtErrorType {
    virtual ~GbtErrorType() = default;
    virtual WinCCResponse createWinCCResponse() = 0;
    static constexpr std::string_view ErrorFile{"GbtErrorsRaport"};
};

struct BCSyncLost : public GbtErrorType {
    BCSyncLost(const std::array<uint8_t, constants::FifoSize>& fifoData);
    [[nodiscard]] WinCCResponse createWinCCResponse() final;
    static constexpr uint32_t getErrorCode() { return 0xEEEE000A; }

    struct Data {
        uint32_t errorCode;
        struct {
            GbtWord data;
            uint16_t counter;
            uint8_t  isData;
        } words[10];
        uint16_t BCCRU;
        uint16_t BCBoard;
        uint32_t orbitBoard;
        uint32_t orbitCRU;
        uint32_t reservedSpace[2];
    } data;
};

struct Unknown : public GbtErrorType {
    Unknown(const std::array<uint8_t, constants::FifoSize>& fifoData);
    [[nodiscard]] WinCCResponse createWinCCResponse();
    static constexpr uint32_t getErrorCode() { return 0x00000000; }

    std::array<uint32_t, constants::FifoSize/sizeof(uint32_t)> data;
};

struct PmEarlyHeader : public GbtErrorType {
    PmEarlyHeader(const std::array<uint8_t, constants::FifoSize>& fifoData);
    [[nodiscard]] WinCCResponse createWinCCResponse();
    static constexpr uint32_t getErrorCode() { return 0xEEEE0009; }

    struct Data {
        uint32_t errorCode;
        GbtWord words[14];
    } data;
};

struct FifoOverload : public GbtErrorType {
    FifoOverload(const std::array<uint8_t, constants::FifoSize>& fifoData);
    [[nodiscard]] WinCCResponse createWinCCResponse();
    static constexpr uint32_t getErrorCode() { return 0xEEEE0008; }

    struct Data {
        uint32_t errorCode;
        GbtWord words[13];
        uint16_t reservedSpace0;
        uint16_t rdRate;
        uint16_t wrRate;
        uint32_t _reservedSpace1;
    } data;
};

[[nodiscard]] std::shared_ptr<GbtErrorType> parseFifoData(const std::array<uint8_t, constants::FifoSize>& fifoData);

class GbtRate
{
   public:
    GbtRate() : oldCount(0) {}
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
        m_timeInterval = std::chrono::duration_cast<std::chrono::milliseconds>(m_currTimePoint - m_lastTimePoint);
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

constexpr std::string_view GbtConfigurationName{ "GBT_DEFAULT" };
constexpr std::string_view GbtConfigurationBoardName{ "GBT" };

} // namespace gbt