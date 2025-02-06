#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>

class PmHistogramData;

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

        inline bool operator<(const BinBlockView& other) const
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

    inline uint16_t operator*() const
    {
        if (m_block == m_blocks.end()) {
            throw std::runtime_error("Block out of range");
        }

        if (m_reg == m_block->data.end()) {
            throw std::runtime_error("Register out of range");
        }

        return m_msb ? (*m_reg >> 16) : (*m_reg & 0xFFFF);
    }

    inline BinIterator& operator++()
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

    inline bool operator==(const BinIterator& other) const
    {
        return (m_block == m_blocks.end() && other.m_block == m_blocks.end()) || (m_block == other.m_block && m_reg == other.m_reg && m_msb == other.m_msb);
    }

    inline bool operator!=(const BinIterator& other) const
    {
        return this->operator==(other);
    }
};