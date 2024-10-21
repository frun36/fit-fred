#include <string_view>
#include <cstdint>

namespace tcm_parameters
{
constexpr std::string_view PmSpiMask{ "BITMASK_PM_LINKS_SPI_ENABLED" };
constexpr std::string_view HighVoltage{ "1.8V_POWER" };
constexpr std::string_view SystemRestarted{ "BOARD_STATUS_SYSTEM_RESTARTED" };
constexpr std::string_view ForceLocalClock{ "BOARD_STATUS_FORCE_LOCAL_CLOCK" };
constexpr std::string_view ResetSystem{ "BOARD_STATUS_RESET_SYSTEM" };
constexpr std::string_view Trigger1Signature{"TRIGGER_1_SIGNATURE"};
constexpr std::string_view Trigger2Signature{"TRIGGER_2_SIGNATURE"};
constexpr std::string_view Trigger3Signature{"TRIGGER_3_SIGNATURE"};
constexpr std::string_view Trigger4Signature{"TRIGGER_4_SIGNATURE"};
constexpr std::string_view Trigger5Signature{"TRIGGER_5_SIGNATURE"};
} // namespace tcm_parameters