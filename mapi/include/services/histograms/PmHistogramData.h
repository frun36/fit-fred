#pragma once

#include <string>
#include <vector>
#include <array>
#include <algorithm>

class BinIterator
{
    friend PmHistogramData;

   private:
    struct BinBlockView {
        const std::vector<uint32_t>& data;
        const bool isNegativeDirection;
        const uint32_t minBin;

        BinBlockView(const std::vector<uint32_t>& data,
                     bool isNegativeDirection,
                     uint32_t minBin)
            : data(data), isNegativeDirection(isNegativeDirection), minBin(minBin) {}

        bool operator<(const BinBlockView& other) const
        {
            return minBin < other.minBin;
        }
    };

    const std::vector<BinBlockView> m_blocks;
    bool m_msb;
    std::vector<BinBlockView>::const_iterator m_block;
    std::vector<uint32_t>::const_iterator m_reg;

    BinIterator(const std::vector<BinBlockView>&& blocks)
        : m_blocks(blocks),
          m_msb(!m_blocks.empty() && m_blocks[0].isNegativeDirection),
          m_block(m_blocks.begin()),
          m_reg(m_blocks.empty() ? std::vector<uint32_t>::const_iterator() : m_block->data.begin()) {}

   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = uint16_t;
    using pointer = const uint16_t*;
    using reference = const uint16_t&;

    uint16_t operator*() const
    {
        if (m_block == m_blocks.end()) {
            throw std::runtime_error("Block out of range");
        }

        if (m_reg == m_block->data.end()) {
            throw std::runtime_error("Register out of range");
        }

        return m_msb ? (*m_reg >> 16) : (*m_reg & 0xFFFF);
    }

    BinIterator& operator++()
    {
        if (m_block == m_blocks.end()) {
            throw std::runtime_error("Iterator already reached end");
        }

        if ((m_block->isNegativeDirection && m_msb) || (!m_block->isNegativeDirection && !m_msb)) {
            m_msb = !m_msb;
            return *this;
        }

        m_reg++;
        if (m_reg != m_block->data.end()) {
            m_msb = !m_msb;
            return *this;
        }

        m_block++;
        if (m_block != m_blocks.end()) {
            m_reg = m_block->data.begin();
        }

        m_msb = !m_msb;
        return *this;
    }

    bool operator==(const BinIterator& other) const
    {
        return (m_block == m_blocks.end() && other.m_block == m_blocks.end()) || (m_block == other.m_block && m_reg == other.m_reg && m_msb == other.m_msb);
    }

    bool operator!=(const BinIterator& other) const
    {
        return this->operator==(other);
    }
};

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

        bool operator<(BinBlock other) const
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

    std::vector<OperationInfo> getOperations() const
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

                // extend previous readout if regblocks are connected (doesn't take into account the auto-skipped region inbetween channel data blocks)
                OperationInfo& lastRequest = requests.back();
                if (currBaseAddress == lastRequest.baseAddress + lastRequest.regblockSize) {
                    lastRequest.regblockSize += block.regblockSize;
                } else {
                    requests.emplace_back(currBaseAddress, block.regblockSize);
                }
            }
        }
    }

    void selectHistograms(std::vector<std::string> names)
    {
        for (auto& channelBlocks : m_channelBlocks) {
            for (auto& bins : channelBlocks) {
                bins.readoutEnabled = (std::find(names.begin(), names.end(), bins.histogramName) != names.end());
            }
        }
    }

    bool storeReadoutData(uint32_t baseAddress, const std::vector<uint32_t>& data)
    {
        // assumes we are inside a single channel
        uint32_t chIdx = baseAddress / ChannelBaseAddress;
        uint32_t relativeAddress = baseAddress % ChannelBaseAddress;

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

    BinIterator getBinIterator(std::string histogramName, uint32_t channelIdx)
    {
        std::vector<BinIterator::BinBlockView> binBlocks;

        const std::vector<BinBlock>& channelBlocks = m_channelBlocks[channelIdx];
        for (const auto& block : channelBlocks) {
            if (block.histogramName == histogramName) {
                binBlocks.emplace_back(
                    block.data,
                    block.isNegativeDirection,
                    block.isNegativeDirection ? block.startBin - block.regblockSize * block.binsPerRegister : block.startBin);
            }
        }

        std::sort(binBlocks.begin(), binBlocks.end());
        return BinIterator(std::move(binBlocks));
    }
};