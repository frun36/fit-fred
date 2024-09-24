#include "ParameterInfo.h"
#include"Parser/utility.h"
#include"utils.h"
ParameterInfo::ParameterInfo(
        std::string name_, 
        uint32_t baseAddress_, 
        uint8_t startBit_, 
        uint8_t bitLength_, 
        uint32_t regBlockSize_, 
        ValueEncoding valueEncoding_,
        double minValue_,
        double maxValue_,
        Equation rawToPhysic_,
        Equation physicToRaw_,
        bool isFifo_,
        bool isReadonly_
    ) : 
        name(name_), 
        baseAddress(baseAddress_), 
        startBit(startBit_), 
        bitLength(bitLength_), 
        regBlockSize(regBlockSize_),
        valueEncoding(valueEncoding_),
        minValue(minValue_),
        maxValue(maxValue_),
        rawToPhysic(equationPIM_),
        physicToRaw(equationPOM_),
        isFifo(isFifo_),
        isReadonly(isReadonly_),
        m_value(0.0) {}


