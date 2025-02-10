#pragma once

#include <string>
#include <vector>
#include <array>
#include <services/histograms/BinIterator.h>
#include <services/histograms/BinBlock.h>

class PmHistogramData
{
   private:
    std::array<std::vector<BinBlock>, 12> m_channelBlocks;
    static constexpr uint32_t ChannelBaseAddress = 0x2000;

   public:
    struct OperationInfo {
        uint32_t baseAddress;
        uint32_t regblockSize;
    };

    std::vector<OperationInfo> getOperations() const;

    void selectHistograms(std::vector<std::string> names);

    bool storeReadoutData(uint32_t baseAddress, const std::vector<uint32_t>& data);

    std::pair<BinIterator, BinIterator> getBeginEndIterators(std::string histogramName, uint32_t channelIdx) const;
};
