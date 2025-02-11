#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include "Database/databaseinterface.h"

struct HistogramInfoRow {
    std::string histogramName;
    uint32_t baseAddress;
    uint32_t regblockSize;
    int32_t startBin;
    uint32_t binsPerRegister;
    bool isNegativeDirection;

    HistogramInfoRow(vector<MultiBase*> data)
    {
        if (data.size() != 6) {
            throw runtime_error("Invalid histogram info data row size");
        }

        histogramName = data[0]->getString();
        istringstream iss(data[1]->getString());
        iss >> hex >> baseAddress;
        regblockSize = data[2]->getDouble();
        startBin = data[3]->getDouble();
        isNegativeDirection = data[4]->getString() == "N";
    }
};

struct BinBlock {
    std::string histogramName;
    uint32_t baseAddress;
    uint32_t regblockSize;
    int32_t startBin;
    uint32_t binsPerRegister;
    bool isNegativeDirection;

    mutable bool readoutEnabled = false;
    mutable std::vector<uint32_t> data;

    BinBlock(std::string histogramName,
             uint32_t baseAddress,
             uint32_t regblockSize,
             int32_t startBin,
             uint32_t binsPerRegister,
             bool isNegativeDirection)
        : histogramName(histogramName),
          baseAddress(baseAddress),
          regblockSize(regblockSize),
          startBin(startBin),
          binsPerRegister(binsPerRegister),
          isNegativeDirection(isNegativeDirection)
    {
        data.resize(regblockSize);
    }

    BinBlock(const HistogramInfoRow& row)
        : BinBlock(row.histogramName, row.baseAddress, row.regblockSize, row.startBin, row.binsPerRegister, row.isNegativeDirection) {}
};
