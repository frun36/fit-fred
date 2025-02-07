#ifndef PM_PARAMETERS
#define PM_PARAMETERS
#include <string_view>
#include <vector>
#include <string>

namespace pm_parameters
{
  
constexpr std::string_view SupplyVoltage1_8V{ "1_8V_POWER" };
constexpr std::string_view Ch01CtrCfd{ "CH01_CTR_CFD" };
constexpr std::string_view Ch01CtrTrg{ "CH01_CTR_TRG" };
constexpr std::string_view Ch02CtrCfd{ "CH02_CTR_CFD" };
constexpr std::string_view Ch02CtrTrg{ "CH02_CTR_TRG" };
constexpr std::string_view Ch03CtrCfd{ "CH03_CTR_CFD" };
constexpr std::string_view Ch03CtrTrg{ "CH03_CTR_TRG" };
constexpr std::string_view Ch04CtrCfd{ "CH04_CTR_CFD" };
constexpr std::string_view Ch04CtrTrg{ "CH04_CTR_TRG" };
constexpr std::string_view Ch05CtrCfd{ "CH05_CTR_CFD" };
constexpr std::string_view Ch05CtrTrg{ "CH05_CTR_TRG" };
constexpr std::string_view Ch06CtrCfd{ "CH06_CTR_CFD" };
constexpr std::string_view Ch06CtrTrg{ "CH06_CTR_TRG" };
constexpr std::string_view Ch07CtrCfd{ "CH07_CTR_CFD" };
constexpr std::string_view Ch07CtrTrg{ "CH07_CTR_TRG" };
constexpr std::string_view Ch08CtrCfd{ "CH08_CTR_CFD" };
constexpr std::string_view Ch08CtrTrg{ "CH08_CTR_TRG" };
constexpr std::string_view Ch09CtrCfd{ "CH09_CTR_CFD" };
constexpr std::string_view Ch09CtrTrg{ "CH09_CTR_TRG" };
constexpr std::string_view Ch10CtrCfd{ "CH10_CTR_CFD" };
constexpr std::string_view Ch10CtrTrg{ "CH10_CTR_TRG" };
constexpr std::string_view Ch11CtrCfd{ "CH11_CTR_CFD" };
constexpr std::string_view Ch11CtrTrg{ "CH11_CTR_TRG" };
constexpr std::string_view Ch12CtrCfd{ "CH12_CTR_CFD" };
constexpr std::string_view Ch12CtrTrg{ "CH12_CTR_TRG" };

constexpr std::string_view ResetCountersAndHistograms{ "RESET_COUNTERS_AND_HISTOGRAMS" };
constexpr std::string_view CounterFifo{ "COUNTERS_VALUES_READOUT" };
constexpr std::string_view CountersFifoLoad{ "COUNTERS_FIFO_LOAD" };

inline std::vector<std::string> getAllCounters()
{
    return {
        std::string{ Ch01CtrCfd }, std::string{ Ch01CtrTrg },
        std::string{ Ch02CtrCfd }, std::string{ Ch02CtrTrg },
        std::string{ Ch03CtrCfd }, std::string{ Ch03CtrTrg },
        std::string{ Ch04CtrCfd }, std::string{ Ch04CtrTrg },
        std::string{ Ch05CtrCfd }, std::string{ Ch05CtrTrg },
        std::string{ Ch06CtrCfd }, std::string{ Ch06CtrTrg },
        std::string{ Ch07CtrCfd }, std::string{ Ch07CtrTrg },
        std::string{ Ch08CtrCfd }, std::string{ Ch08CtrTrg },
        std::string{ Ch09CtrCfd }, std::string{ Ch09CtrTrg },
        std::string{ Ch10CtrCfd }, std::string{ Ch10CtrTrg },
        std::string{ Ch11CtrCfd }, std::string{ Ch11CtrTrg },
        std::string{ Ch12CtrCfd }, std::string{ Ch12CtrTrg }
    };
}

constexpr std::string_view ResetHistograms { "HIST_RESET" };
constexpr std::string_view HistogrammingOn { "HIST_ON" };
constexpr std::string_view CurrentAddressInHistogramData { "CURRENT_ADDRESS_IN_HISTOGRAM_DATA" };
constexpr std::string_view HistogramDataReadout { "HISTOGRAM_DATA_READOUT" };

} // namespace pm_parameters
#endif
