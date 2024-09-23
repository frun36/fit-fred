#include "ParameterInfo.h"


double ParameterInfo::getPhysicalValue(uint32_t rawValue) const {
    uint8_t bitLength = m_endBit - m_startBit + 1;
    uint32_t shiftedValue = (rawValue >> m_startBit) & ((1U << bitLength) - 1);
    
    double encodedValue;
    switch (m_valueEncoding) {
        case ValueEncoding::Unsigned:
            encodedValue = static_cast<double>(shiftedValue);
            break;
        case ValueEncoding::Signed32:
            encodedValue = fromNBitSigned(shiftedValue, 32);
            break;
        case ValueEncoding::Signed16:
            encodedValue = fromNBitSigned(shiftedValue, 16);
            break;
    }
    return m_multiplier * encodedValue;
}

uint32_t ParameterInfo::getRawValue(double physicalValue) const {
    double encodedValue = physicalValue / m_multiplier;

    switch (m_valueEncoding) {
        case ValueEncoding::Unsigned:
            return static_cast<uint32_t>(encodedValue) << m_startBit;
        case ValueEncoding::Signed32:
            return asNBitSigned(encodedValue, 32) << m_startBit;
        case ValueEncoding::Signed16:
            return asNBitSigned(encodedValue, 16) << m_startBit;
        default:
            throw std::runtime_error("Invalid value encoding");
    }
}