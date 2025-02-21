#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "FitData.h"

struct BinBlock {
    std::string histogramName;
    uint32_t baseAddress;
    uint32_t regBlockSize;
    int32_t startBin;
    uint32_t binsPerRegister;
    bool isNegativeDirection;

    mutable bool readoutEnabled = false;
    mutable std::vector<uint32_t> data;

    BinBlock(std::string histogramName,
             uint32_t baseAddress,
             uint32_t regBlockSize,
             int32_t startBin,
             uint32_t binsPerRegister,
             bool isNegativeDirection)
        : histogramName(histogramName),
          baseAddress(baseAddress),
          regBlockSize(regBlockSize),
          startBin(startBin),
          binsPerRegister(binsPerRegister),
          isNegativeDirection(isNegativeDirection)
    {
        data.resize(regBlockSize);
    }

    BinBlock(const std::string& histogramName, const FitData::PmHistogramBlock& row)
        : BinBlock(histogramName, row.baseAddress, row.regBlockSize, row.startBin, row.binsPerRegister, row.direction == FitData::PmHistogramBlock::Direction::Negative) {}
};
