#include <string_view>
#include <cstdint>

namespace tcm_parameters
{
constexpr std::string_view PmSpiMask{ "BITMASK_PM_LINKS_SPI_ENABLED" };
constexpr std::string_view HighVoltage{ "1.8V_POWER" };
constexpr std::string_view SystemRestarted{ "BOARD_STATUS_SYSTEM_RESTARTED" };
constexpr std::string_view ForceLocalClock{ "BOARD_STATUS_FORCE_LOCAL_CLOCK" };
constexpr std::string_view ResetSystem{ "BOARD_STATUS_RESET_SYSTEM" };
} // namespace tcm_parameters