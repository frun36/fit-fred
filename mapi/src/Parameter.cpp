#include "Parameter.h"


double Parameter::getPhysicalValue(uint32_t rawValue) const {
    uint8_t bitLength = m_endBit - m_startBit + 1;
    uint32_t shiftedValue = (rawValue >> m_startBit) & ((1 << bitLength) - 1);
    
    double encodedValue;
    switch (m_valueEncoding) {
        case ValueEncoding::Unsigned:
            encodedValue = static_cast<double>(shiftedValue);
            break;
        case ValueEncoding::Signed32:
            encodedValue = static_cast<double>(static_cast<int32_t>(shiftedValue));
            break;
        case ValueEncoding::Signed16:
            encodedValue = static_cast<double>(static_cast<int16_t>(shiftedValue));
            break;
    }
    return m_multiplier * encodedValue;
    }

uint32_t Parameter::getRawValue(double physicalValue) {
    double encodedValue = physicalValue / m_multiplier;

    switch (m_valueEncoding) {
        case ValueEncoding::Unsigned:
            return static_cast<uint32_t>(encodedValue) << m_startBit;
        case ValueEncoding::Signed32:
            return static_cast<uint32_t>(static_cast<int32_t>(encodedValue)) << m_startBit;
        case ValueEncoding::Signed16:
            return static_cast<uint32_t>(static_cast<int16_t>(encodedValue)) << m_startBit;
    }
}