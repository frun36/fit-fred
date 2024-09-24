#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

// #include <Database/databaseinterface.h>

struct ParameterInfo {
    // The encoding system requires serious rethinking, based on the electronics
    enum class ValueEncoding {
        Unsigned, Signed
    };

    ParameterInfo() = default;
    ParameterInfo(
        std::string name, 
        uint32_t baseAddress, 
        uint8_t startBit, 
        uint8_t bitLength, 
        uint32_t regblockSize, 
        ValueEncoding valueEncoding,
        double minValue,
        double maxValue,
        double multiplier,
        bool isFifo,
        bool isReadonly
    ) : 
        name(name), 
        baseAddress(baseAddress), 
        startBit(startBit), 
        bitLength(bitLength), 
        regBlockSize(regBlockSize),
        valueEncoding(valueEncoding),
        minValue(minValue),
        maxValue(maxValue),
        multiplier(multiplier),
        isFifo(isFifo),
        isReadonly(isReadonly),
        m_value(0.0) {}

    const std::string name{0};
    const uint32_t baseAddress{0};
    const uint8_t startBit{0};
    const uint8_t bitLength{0};
    const size_t regBlockSize{1};
    const ValueEncoding valueEncoding{ValueEncoding::Unsigned};
    const double minValue{0};
    const double maxValue{0};
    const double multiplier{0};
    const bool isFifo{false};
    const bool isReadonly{false};

    double calculatePhysicalValue(uint32_t rawValue) const;
    uint32_t calculateRawValue(double physicalValue) const;

    void storeValue(double value){m_value = value;}
    double getStoredValue() const {return m_value;}

public:
    double m_value;
};