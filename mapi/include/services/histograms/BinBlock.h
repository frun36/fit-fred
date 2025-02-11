#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "Database/databaseinterface.h"

struct HistogramInfoRow {
    std::string histogramName;
    uint32_t baseAddress;
    int32_t startBin;
    uint32_t regblockSize;
    bool isNegativeDirection;

    HistogramInfoRow(vector<MultiBase*> data)
    {
        if (data.size() != 5) {
            throw runtime_error("Invalid histogram info data row size");
        }

        histogramName = data[0]->getString();
        baseAddress = data[1]->getInt(); // parse from hex to int - todo
        startBin = data[2]->getDouble();
        regblockSize = data[3]->getDouble();
        isNegativeDirection = data[4]->getString() == "Y";
    }
};

struct BinBlock {
    std::string histogramName;
    uint8_t binsPerRegister = 2;
    uint32_t baseAddress;
    int32_t startBin;
    uint32_t regblockSize;
    bool isNegativeDirection;

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

    BinBlock(const HistogramInfoRow& row)
        : BinBlock(row.histogramName, row.baseAddress, row.startBin, row.regblockSize, row.isNegativeDirection) {}
};
