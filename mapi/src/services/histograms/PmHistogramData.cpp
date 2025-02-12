#include "services/histograms/PmHistogramData.h"
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <set>
#include <sstream>
#include <vector>
#include "FitData.h"
#include "services/histograms/BinBlock.h"
#include <database/sql.h>
#include <Database/databaseinterface.h>
#include <Alfred/print.h>

std::array<std::vector<BinBlock>, 12> PmHistogramData::fetchChannelBlocks(std::unordered_map<std::string, FitData::PmHistogram> histograms)
{
    std::vector<BinBlock> channelBlocks;
    for (const auto& [name, hist] : histograms) {
        channelBlocks.emplace_back(name, *hist.negativeBins);
        channelBlocks.emplace_back(name, *hist.positiveBins);
    }

    std::sort(channelBlocks.begin(), channelBlocks.end(), [](const auto& a, const auto& b) {
        return a.baseAddress < b.baseAddress;
    });

    std::array<std::vector<BinBlock>, 12> channels;
    std::for_each(channels.begin(), channels.end(), [channelBlocks](auto& ch) { ch = channelBlocks; });
    return channels;
}

BlockView PmHistogramData::createBlockView()
{
    BlockView view;
    for (size_t chIdx = 0; chIdx < 12; chIdx++) {
        for (const auto& block : m_channelBlocks[chIdx]) {
            std::stringstream key;
            key << "CH" << std::setw(2) << std::setfill('0') << chIdx + 1 << block.histogramName;
            view[key.str()].push_back(&block);
        }
    }
    for (auto& [key, blocks] : view) {
        std::sort(
            blocks.begin(),
            blocks.end(),
            [](const BinBlock* a, const BinBlock* b) {
                return a->startBin < b->startBin; // blocks are disjoint - true min bin doesn't need to be calculated
            });
    }
    return view;
}

void PmHistogramData::updateOperations()
{
    std::vector<OperationInfo> operations;

    for (size_t chIdx = 0; chIdx < 12; chIdx++) {
        const std::vector<BinBlock>& channel = m_channelBlocks[chIdx];
        for (const auto& block : channel) {
            if (!block.readoutEnabled) {
                continue;
            }

            uint32_t currBaseAddress = chIdx * ChannelBaseAddress + block.baseAddress;
            if (operations.empty()) {
                operations.emplace_back(currBaseAddress, block.regBlockSize);
                continue;
            }

            // extend previous readout if regblocks are connected
            // (works only within single channel)
            OperationInfo& lastRequest = operations.back();
            if (currBaseAddress == lastRequest.baseAddress + lastRequest.regblockSize) {
                lastRequest.regblockSize += block.regBlockSize;
            } else {
                operations.emplace_back(currBaseAddress, block.regBlockSize);
            }
        }
    }
    m_operations = operations;
    for (auto o : m_operations) {
        Print::PrintData("Operation: " + to_string(o.baseAddress) + " " + to_string(o.regblockSize));
    }
}

std::string PmHistogramData::selectHistograms(std::vector<std::string> names)
{
    std::set<std::string> enabledNames;
    for (auto& channelBlocks : m_channelBlocks) {
        for (auto& bins : channelBlocks) {
            bins.readoutEnabled = (std::find(names.begin(), names.end(), bins.histogramName) != names.end());
            if (bins.readoutEnabled) {
                enabledNames.insert(bins.histogramName);
            }
        }
    }
    updateOperations();
    return std::accumulate(enabledNames.begin(), enabledNames.end(), std::string(""), [](const std::string& acc, const std::string& name) {
        return acc + name + "; ";
    });
}

bool PmHistogramData::storeReadoutData(uint32_t baseAddress, const std::vector<uint32_t>& data)
{
    // assumes we are inside a single channel
    uint32_t chIdx = baseAddress / ChannelBaseAddress;
    uint32_t relativeAddress = baseAddress % ChannelBaseAddress;
    Print::PrintData("CH idx: " + to_string(chIdx) + ", base address " + to_string(baseAddress) + ", readout len " + to_string(data.size()));

    const std::vector<BinBlock>& blocks = m_channelBlocks[chIdx];
    // we know the result from a single operation is a contiguous block of data
    size_t currDataIdx = 0;
    for (auto& block : blocks) {
        Print::PrintData("Block: " + block.histogramName + " " + to_string(block.baseAddress) + " " + to_string(block.regBlockSize) + ", position in data: " + to_string(relativeAddress + currDataIdx));
        if (!(relativeAddress + currDataIdx >= block.baseAddress && relativeAddress + currDataIdx < block.baseAddress + block.regBlockSize)) {
            continue;
        }

        auto begin = block.data.begin() + relativeAddress + currDataIdx - block.baseAddress;
        size_t copyLen = block.data.end() - begin;
        Print::PrintData("Copy len: " + std::to_string(copyLen));
        if (data.begin() + currDataIdx >= data.end() || data.begin() + currDataIdx + copyLen > data.end()) {
            Print::PrintError("Unexpected: histogram readout data storage requires copy out of data vector range");
            return false;
        }
        std::copy(data.begin() + currDataIdx, data.begin() + currDataIdx + copyLen, begin);
        currDataIdx += copyLen;
        if (currDataIdx >= data.size()) {
            break;
        }
    }

    return currDataIdx == data.size();
}
