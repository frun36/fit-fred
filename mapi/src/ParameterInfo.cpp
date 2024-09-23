#include "ParameterInfo.h"
#include"Utils.h"

double ParameterInfo::getPhysicalValue(uint32_t rawValue) const {
    rawValue = getBitField(rawValue, m_startBit, m_endBit);
    if(m_valueEncoding != ValueEncoding::Unsigned)
    {
        return m_multiplier * static_cast<double>(TwoComplementsDecode<int32_t>(rawValue, m_endBit - m_startBit + 1));
    }
    else  return m_multiplier * static_cast<double>(rawValue);
}

uint32_t ParameterInfo::getRawValue(double physicalValue) const {
    double scaled = physicalValue / m_multiplier;
    if(scaled > maxUINT(m_startBit, m_endBit))
    {
        throw std::runtime_error("Physical value is too large to fit in the bit field");
    }
    int32_t singedValue = static_cast<int32_t>(scaled);
    if(m_valueEncoding != ValueEncoding::Unsigned)
    {
       return TwoComplementsEncode(singedValue, m_endBit - m_startBit + 1) << m_startBit;
    }
    return  static_cast<uint32_t>(singedValue) << m_startBit;
}