#include "ParameterInfo.h"
#include"utils.h"

double ParameterInfo::calculatePhysicalValue(uint32_t rawValue) const {
    rawValue = getBitField(rawValue, startBit, bitLength);
    if(valueEncoding != ValueEncoding::Unsigned)
        return multiplier * static_cast<double>(twosComplementDecode<int32_t>(rawValue, bitLength));
    else
        return multiplier * static_cast<double>(rawValue);
}

uint32_t ParameterInfo::calculateRawValue(double physicalValue) const {
    double scaled = physicalValue / multiplier;
    if(scaled > ((1u << bitLength) - 1u))
        throw std::runtime_error(name + ": physical value is too large to fit in the bit field");

    int32_t singedValue = static_cast<int32_t>(scaled);
    if(valueEncoding != ValueEncoding::Unsigned)
       return twosComplementEncode(singedValue, bitLength) << startBit;

    return  static_cast<uint32_t>(singedValue) << startBit;
}