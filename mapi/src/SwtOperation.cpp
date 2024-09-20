#include "SwtOperation.h"

SwtOperation SwtOperation::fromParameterOperation(const Parameter& parameter, WinCCRequest::Command::Operation operation, std::optional<double> data) {
    const std::string& parameterName = parameter.getName();
    uint32_t baseAddress = parameter.getBaseAddress();
    uint32_t regblockSize = parameter.getRegblockSize();

    if (regblockSize != 1)
        throw std::runtime_error(parameterName + ": regblock operations unsupported");

    uint8_t startBit = parameter.getStartBit();
    uint8_t endBit = parameter.getEndBit();
    uint8_t bitLength = parameter.getBitLength();

    if (operation == WinCCRequest::Command::Operation::Read)
        return SwtOperation(SwtOperation::Type::Read, baseAddress);

    // WRITE operation
    if(!data.has_value())
        throw std::runtime_error(parameterName + ": no data for WRITE operation");
    
    uint32_t rawValue = parameter.getRawValue(data.value());


    if (bitLength == 32)
        return SwtOperation(SwtOperation::Type::Write, baseAddress, rawValue);

    // needs RMW
    uint32_t andMask = ~(((1 << bitLength) - 1) << startBit);
    uint32_t orMask = rawValue;
    return SwtOperation(SwtOperation::Type::RmwBits, baseAddress, andMask, orMask);
}