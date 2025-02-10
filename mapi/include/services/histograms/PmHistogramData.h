#pragma once

#include <map>
#include <string>
#include <vector>
#include <array>
#include <services/histograms/BinBlock.h>

using BlocksView = std::map<std::string, std::vector<const BinBlock*>>; 

class PmHistogramData
{
   private:
    const std::array<std::vector<BinBlock>, 12> m_channelBlocks;
    const BlocksView m_orderedBlocksView;
    static constexpr uint32_t ChannelBaseAddress = 0x2000;

    static std::array<std::vector<BinBlock>, 12> fetchChannelBlocks()
    {
        // todo
    }

    BlocksView createBlocksView();

   public:
    struct OperationInfo {
        uint32_t baseAddress;
        uint32_t regblockSize;
    };

    std::vector<OperationInfo> getOperations() const;

    void selectHistograms(std::vector<std::string> names);

    bool storeReadoutData(uint32_t baseAddress, const std::vector<uint32_t>& data);

    PmHistogramData() : m_channelBlocks(fetchChannelBlocks()), m_orderedBlocksView(createBlocksView()) {}

    const BlocksView& getData() const {
        return m_orderedBlocksView;
    }
};
