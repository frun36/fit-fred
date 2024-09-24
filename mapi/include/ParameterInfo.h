#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

// #include <Database/databaseinterface.h>

class ParameterInfo {
public:
    // The encoding system requires serious rethinking, based on the electronics
    enum class ValueEncoding {
        Unsigned, Signed
    };

private:
    std::string m_name;
    uint32_t m_baseAddress;
    uint8_t m_startBit;
    uint8_t m_endBit;
    uint32_t m_regblockSize;
    ValueEncoding m_valueEncoding;
    double m_minValue;
    double m_maxValue;
    double m_multiplier;
    bool m_isFifo;
    bool m_isReadonly;

public:
    // ParameterInfo(std::vector<MultiBase*>) {
    //     // to be implemented when database structure is certain
    // }

    ParameterInfo() = default;

    ParameterInfo(
        std::string name, 
        uint32_t baseAddress, 
        uint8_t startBit, 
        uint8_t endBit, 
        uint32_t regblockSize, 
        ValueEncoding valueEncoding,
        double minValue,
        double maxValue,
        double multiplier,
        bool isFifo,
        bool isReadonly
    ) : 
        m_name(name), 
        m_baseAddress(baseAddress), 
        m_startBit(startBit), 
        m_endBit(endBit), 
        m_regblockSize(regblockSize),
        m_valueEncoding(valueEncoding),
        m_minValue(minValue),
        m_maxValue(maxValue),
        m_multiplier(multiplier),
        m_isFifo(isFifo),
        m_isReadonly(isReadonly) {}

    const std::string& getName() const {
        return m_name;
    }

    uint32_t getBaseAddress() const {
        return m_baseAddress;
    }

    uint8_t getStartBit() const {
        return m_startBit;
    }

    uint8_t getEndBit() const {
        return m_endBit;
    }

    uint8_t getBitLength() const {
        return m_endBit - m_startBit + 1;
    }

    uint32_t getRegblockSize() const {
        return m_regblockSize;
    }

    double getPhysicalValue(uint32_t rawValue) const;

    uint32_t getRawValue(double physicalValue) const;

    // Currently no value validation!

    bool isFifo() const {
        return m_isFifo;
    }

    bool isReadonly() const {
        return m_isReadonly;
    }
};