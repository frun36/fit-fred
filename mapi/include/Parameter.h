#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

#include <Database/databaseinterface.h>

class Parameter {
public:
    enum class ValueEncoding {
        Unsigned, Signed16, Signed32
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
    Parameter(std::vector<MultiBase*>) {
        // to be implemented when database structure is certain
    }

    Parameter(
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

    uint32_t getRegblockSize() const {
        return m_regblockSize;
    }

    double getPhysicalValue(uint32_t rawValue) const;

    uint32_t getRawValue(double physicalValue);

    // Currently no value validation!

    bool isFifo() const {
        return m_isFifo;
    }

    bool isReadonly() const {
        return m_isReadonly;
    }
};