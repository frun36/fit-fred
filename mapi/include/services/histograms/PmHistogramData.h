#pragma once

#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <vector>
#include <array>
#include "Board.h"
#include "PM.h"
#include <services/histograms/BinBlock.h>

using BlockView = std::map<std::string, std::vector<const BinBlock*>>;

class PmHistogramData
{
   public:
    struct OperationInfo {
        uint32_t baseAddress;
        uint32_t regblockSize;

        OperationInfo(uint32_t baseAddress, uint32_t regblockSize) : baseAddress(baseAddress), regblockSize(regblockSize) {}
    };

    std::string selectHistograms(std::vector<std::string> names);

    bool storeReadoutData(uint32_t baseAddress, const std::vector<uint32_t>& data);

    PmHistogramData(shared_ptr<Board> pm, std::unordered_map<std::string, FitData::PmHistogram> histograms)
        : m_channelBlocks(fetchChannelBlocks(histograms)),
          m_orderedBlocksView(createBlockView()),
          ChannelBaseAddress(!pm->isTcm() ? pm->at(pm_parameters::HistogramReadout).regBlockSize : 0) {
        updateOperations();
    }

    const vector<OperationInfo>& getOperations() const
    {
        return m_operations;
    }

    const BlockView& getData() const
    {
        return m_orderedBlocksView;
    }

    size_t getTotalBins() const
    {
        return std::accumulate(m_channelBlocks.begin(), m_channelBlocks.end(), 0, [](size_t acc, const std::vector<BinBlock> ch) {
            return acc + std::accumulate(ch.begin(), ch.end(), 0, [](size_t chAcc, const BinBlock& block) {
                       return chAcc + block.regBlockSize * block.binsPerRegister;
                   });
        });
    }

   private:
    const std::array<std::vector<BinBlock>, 12> m_channelBlocks;
    const BlockView m_orderedBlocksView;
    std::vector<OperationInfo> m_operations;
    const uint32_t ChannelBaseAddress;

    static std::array<std::vector<BinBlock>, 12> fetchChannelBlocks(std::unordered_map<std::string, FitData::PmHistogram> histograms);
    BlockView createBlockView();
    void updateOperations();
};
