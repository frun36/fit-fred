#ifndef TCM_PARAMETERS
#define TCM_PARAMETERS

#include <string_view>
#include <vector>
#include <string>

namespace tcm_parameters
{
constexpr std::string_view PmSpiMask{ "BITMASK_PM_LINKS_SPI_ENABLED" };
constexpr std::string_view SupplyVoltage1_8V{ "1.8V_POWER" };
constexpr std::string_view SystemRestarted{ "BOARD_STATUS_SYSTEM_RESTARTED" };
constexpr std::string_view ForceLocalClock{ "BOARD_STATUS_FORCE_LOCAL_CLOCK" };
constexpr std::string_view ResetSystem{ "BOARD_STATUS_RESET_SYSTEM" };
constexpr std::string_view Trigger1Signature{ "TRIGGER_1_SIGNATURE" };
constexpr std::string_view Trigger2Signature{ "TRIGGER_2_SIGNATURE" };
constexpr std::string_view Trigger3Signature{ "TRIGGER_3_SIGNATURE" };
constexpr std::string_view Trigger4Signature{ "TRIGGER_4_SIGNATURE" };
constexpr std::string_view Trigger5Signature{ "TRIGGER_5_SIGNATURE" };
constexpr std::string_view DelayA{ "DELAY_A" };
constexpr std::string_view DelayC{ "DELAY_C" };

constexpr std::string_view Trigger5{ "TRIGGER_5_COUNTER" };
constexpr std::string_view Trigger4{ "TRIGGER_4_COUNTER" };
constexpr std::string_view Trigger3{ "TRIGGER_3_COUNTER" };
constexpr std::string_view Trigger2{ "TRIGGER_2_COUNTER" };
constexpr std::string_view Trigger1{ "TRIGGER_1_COUNTER" };
constexpr std::string_view Background0{ "BACKGROUND_0_COUNTER" };
constexpr std::string_view Background1{ "BACKGROUND_1_COUNTER" };
constexpr std::string_view Background2{ "BACKGROUND_2_COUNTER" };
constexpr std::string_view Background3{ "BACKGROUND_3_COUNTER" };
constexpr std::string_view Background4{ "BACKGROUND_4_COUNTER" };
constexpr std::string_view Background5{ "BACKGROUND_5_COUNTER" };
constexpr std::string_view Background6{ "BACKGROUND_6_COUNTER" };
constexpr std::string_view Background7{ "BACKGROUND_7_COUNTER" };
constexpr std::string_view Background8{ "BACKGROUND_8_COUNTER" };
constexpr std::string_view Background9{ "BACKGROUND_9_COUNTER" };

constexpr std::string_view CounterReadInterval{ "COUNTER_READ_INTERVAL" };
constexpr std::string_view ResetCounters{ "BOARD_STATUS_RESET_COUNTERS" };
constexpr std::string_view CounterFifo{ "COUNTERS_VALUES_READOUT" };
constexpr std::string_view CountersFifoLoad{ "COUNTERS_FIFO_LOAD" };

constexpr std::string_view CorrelationCountersSelectable{ "HISTOGRAM_CORRELATION_COUNTERS_SELECTABLE" };
constexpr std::string_view CorrelationCountersOrA{ "HISTOGRAM_CORRELATION_COUNTERS_OR_A" };
constexpr std::string_view CorrelationCountersOrC{ "HISTOGRAM_CORRELATION_COUNTERS_OR_C" };
constexpr std::string_view CorrelationCountersSelect{ "MODE_CORRELATION_COUNTER_SELECT" };
constexpr std::string_view ResetHistograms{ "MODE_RESET_PER_BC_TRIGGER_COUNTER" };

inline std::vector<std::string> getAllCounters()
{
    return { Trigger5.data(), Trigger4.data(), Trigger2.data(), Trigger1.data(), Trigger3.data(),
             Background0.data(), Background1.data(), Background2.data(), Background3.data(), Background4.data(), Background5.data(),
             Background6.data(), Background7.data(), Background8.data(), Background9.data() };
}

inline std::vector<std::string_view> getAllHistograms()
{
    return { CorrelationCountersSelectable, CorrelationCountersOrA, CorrelationCountersOrC };
}

} // namespace tcm_parameters
#endif
