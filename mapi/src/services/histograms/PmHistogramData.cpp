#include "services/histograms/PmHistogramData.h"
#include <algorithm>

std::vector<PmHistogramData::OperationInfo> PmHistogramData::getOperations() const
{
    std::vector<OperationInfo> requests; // (baseAdress, readSize)

    for (size_t chIdx = 0; chIdx < 12; chIdx++) {
        const std::vector<BinBlock>& channel = m_channelBlocks[chIdx];
        for (const auto& block : channel) {
            if (!block.readoutEnabled) {
                continue;
            }

            uint32_t currBaseAddress = chIdx * ChannelBaseAddress + block.baseAddress;
            if (requests.empty()) {
                requests.emplace_back(currBaseAddress, block.regblockSize);
                continue;
            }

            // extend previous readout if regblocks are connected
            // (doesn't take into account the auto-skipped region inbetween channel data blocks)
            OperationInfo& lastRequest = requests.back();
            if (currBaseAddress == lastRequest.baseAddress + lastRequest.regblockSize) {
                lastRequest.regblockSize += block.regblockSize;
            } else {
                requests.emplace_back(currBaseAddress, block.regblockSize);
            }
        }
    }
    return requests;
}

void PmHistogramData::selectHistograms(std::vector<std::string> names)
{
    for (auto& channelBlocks : m_channelBlocks) {
        for (auto& bins : channelBlocks) {
            bins.readoutEnabled = (std::find(names.begin(), names.end(), bins.histogramName) != names.end());
        }
    }
}

bool PmHistogramData::storeReadoutData(uint32_t baseAddress, const std::vector<uint32_t>& data)
{
    // assumes we are inside a single channel
    uint32_t chIdx = baseAddress / ChannelBaseAddress;

    std::vector<BinBlock>& blocks = m_channelBlocks[chIdx];
    // we know the result from a single operation is a contiguous block of data
    size_t currDataIdx = 0;
    for (auto& block : blocks) {
        if (!(baseAddress + currDataIdx >= block.baseAddress && baseAddress + currDataIdx < block.baseAddress + block.regblockSize)) {
            continue;
        }

        auto begin = block.data.begin() + baseAddress + currDataIdx - block.baseAddress;
        size_t copyLen = data.end() - begin;
        std::copy(data.begin() + currDataIdx, data.begin() + currDataIdx + copyLen, begin);
        currDataIdx += copyLen;
    }

    return currDataIdx == data.size();
}

std::pair<BinIterator, BinIterator> PmHistogramData::getBeginEndIterators(std::string histogramName, uint32_t channelIdx) const
{
    std::vector<std::vector<BinBlock>::const_iterator> blockIterators;

    const std::vector<BinBlock>& channelBlocks = m_channelBlocks[channelIdx];
    for (auto it = channelBlocks.begin(); it < channelBlocks.end(); it++) {
        if (it->histogramName == histogramName) {
            blockIterators.emplace_back(it);
        }
    }

    std::sort(blockIterators.begin(), blockIterators.end(), [](auto a, auto b) {
        return a->startBlock < b->startBlock; // blocks are disjoint, no need to calculate actual smallest bin
    });
    return { BinIterator(std::move(blockIterators)), BinIterator({}) };
}