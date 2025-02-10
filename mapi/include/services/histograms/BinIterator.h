#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include "services/histograms/BinBlock.h"

class PmHistogramData;

class BinIterator
{
    friend PmHistogramData;

   private:
    std::vector<std::vector<BinBlock>::const_iterator> m_blocks;
    std::vector<std::vector<BinBlock>::const_iterator>::const_iterator m_block;
    std::vector<uint32_t>::const_iterator m_reg;
    bool m_lsb;

    // blocks empty - end iterator
    BinIterator(std::vector<std::vector<BinBlock>::const_iterator>&& blocks)
        : m_blocks(blocks),
          m_block(m_blocks.begin()),
          m_reg(m_blocks.empty() ? std::vector<uint32_t>::const_iterator()
                                 : ((**m_block).isNegativeDirection ? (**m_block).data.end() - 1
                                                                    : (**m_block).data.begin())),
          m_lsb(!m_blocks.empty() && !m_blocks[0]->isNegativeDirection) {}

   public:
    inline BinIterator& operator++()
    {
        if (m_block == m_blocks.end()) {
            throw std::runtime_error("Iterator has already reached end");
        }

        // Increment within single register
        if (((**m_block).isNegativeDirection && !m_lsb) || ((**m_block).isNegativeDirection && m_lsb)) {
            m_lsb = !m_lsb;
            return *this;
        }

        // for positive direction bins
        if (!(**m_block).isNegativeDirection) {
            m_reg++;
            if (m_reg != (**m_block).data.end()) {
                m_lsb = !m_lsb;
                return *this;
            }
        } else { // for negative direction bins
            m_reg--;
            if (m_reg >= (**m_block).data.begin()) {
                m_lsb = !m_lsb;
                return *this;
            }
        }

        m_block++;
        if (m_block != m_blocks.end()) {
            m_reg = (**m_block).isNegativeDirection ? (**m_block).data.end() - 1 : (**m_block).data.begin();
        }
        m_lsb = !m_lsb;
        return *this;
    }

    inline uint16_t operator*() const
    {
        if (m_block == m_blocks.end()) {
            throw std::runtime_error("Iterator has already reached end");
        } else if (m_reg < (**m_block).data.begin() || m_reg >= (**m_block).data.end()) {
            throw std::runtime_error("Unexpected: exceeded regblock");
        }

        return m_lsb ? (*m_reg & 0xFFFF) : (*m_reg >> 16);
    }

    // assuming they are called on the same vector, or an empty vector symbolyzing end
    inline bool operator==(const BinIterator& o) const
    {
        return (o.m_block == o.m_blocks.end() && m_block == m_blocks.end()) || (o.m_block != o.m_blocks.end() && m_block != m_blocks.end() && *m_block == *o.m_block);
    }

    inline bool operator!=(const BinIterator& o) const
    {
        return !operator==(o);
    }
};
