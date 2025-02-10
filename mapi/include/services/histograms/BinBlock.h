#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct BinBlock {
    const std::string histogramName;
    const uint8_t binsPerRegister = 2;
    const uint32_t baseAddress;
    const int32_t startBin;
    const uint32_t regblockSize;
    const bool isNegativeDirection;

    mutable bool readoutEnabled = false;
    mutable std::vector<uint32_t> data;

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
