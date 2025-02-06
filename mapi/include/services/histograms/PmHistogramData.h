#pragma once

#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <services/histograms/BinIterator.h>

class PmHistogramData
{
   private:
    struct BinBlock {
        const std::string histogramName;
        const uint8_t binsPerRegister = 2;
        const uint32_t baseAddress;
        const int32_t startBin;
        const uint32_t regblockSize;
        const bool isNegativeDirection;

        bool readoutEnabled = false;
        std::vector<uint32_t> data;

        inline bool operator<(BinBlock other) const
        {
            return baseAddress < other.baseAddress;
        }

        BinBlock(std::string histogramName,
                 uint32_t baseAddress,
                 int32_t startBin,
                 uint32_t regblockSize,
                 bool isNegativeDirection)
            : histogramName(histogramName), baseAddress(baseAddress), startBin(startBin), regblockSize(regblockSize), isNegativeDirection(isNegativeDirection)
        {
            data.resize(regblockSize);
        }
    };

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

    BinIterator getBinIterator(std::string histogramName, uint32_t channelIdx) const;
};