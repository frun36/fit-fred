#include"GBT.h"
#include<cstring>
namespace gbt_error
{
[[nodiscard]] std::shared_ptr<GbtErrorType> parseFifoData(const std::array<uint32_t, constants::FifoSize>& fifoData)
{
    switch(fifoData[0])
    {
        case BCSyncLost::getErrorCode():
            return std::shared_ptr<GbtErrorType>(new BCSyncLost(fifoData));

        case FifoOverload::getErrorCode():
            return std::shared_ptr<GbtErrorType>(new FifoOverload(fifoData));

        case PmEarlyHeader::getErrorCode():
            return std::shared_ptr<GbtErrorType>(new PmEarlyHeader(fifoData));

        default:
            throw std::runtime_error("Unsupported GBT ERROR!");
    }
}

BCSyncLost::BCSyncLost(const std::array<uint32_t, constants::FifoSize>& fifoData)
{
    std::memcpy((uint32_t*) &data, fifoData.data(), sizeof(Data));
}

WinCCResponse BCSyncLost::createWinCCResponse()
{
    WinCCResponse response;
    response.addParameter(gbt_error::parameters::BCSyncLost.data(), {1});
    return std::move(response);
}

PmEarlyHeader::PmEarlyHeader(const std::array<uint32_t, constants::FifoSize>& fifoData)
{
    std::memcpy((uint32_t*) &data, fifoData.data(), sizeof(Data));
}

WinCCResponse PmEarlyHeader::createWinCCResponse()
{
    WinCCResponse response;
    response.addParameter(gbt_error::parameters::PmEarlyHeader.data(), {1});
    return std::move(response);
}

FifoOverload::FifoOverload(const std::array<uint32_t, constants::FifoSize>& fifoData)
{
    std::memcpy((uint32_t*) &data, fifoData.data(), sizeof(Data));
}

WinCCResponse FifoOverload::createWinCCResponse()
{
    WinCCResponse response;
    response.addParameter(gbt_error::parameters::FifoOverload.data(), {1});
    return std::move(response);
}

}

namespace gbt_rate
{
    WinCCResponse GbtRateMonitor::updateRates(uint32_t wordsCount, uint32_t eventsCount)
    {
        WinCCResponse response;
        response.addParameter(parameters::WordsRate.data(), {m_wordsRate.updateRate(wordsCount, m_timeInterval)});
        response.addParameter(parameters::EventsRate.data(), {m_eventsRate.updateRate(eventsCount, m_timeInterval)});
        return response;
    }

    double GbtRate::updateRate(uint32_t newCount, std::chrono::milliseconds timeInterval)
    {
        double rate = (newCount >= oldCount) ? static_cast<double>(newCount - oldCount)/static_cast<double>(timeInterval.count()): newCount;
        oldCount = newCount;
        return rate;
    }

}