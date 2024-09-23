#include "Parameters.h"
#include "WinCCRequest.h"
#include "SwtSequence.h"
#include "AlfResponseParser.h"
#include "WinCCResponse.h"
#include <algorithm>

string Parameters::processInputMessage(string msg) {
    try {
        WinCCRequest req(msg);
        vector<SwtSequence::SwtOperation> operations = handleRequest(req);
        // SwtSequence seq(operations);
        SwtSequence seq;
        for (const auto& op : operations)
            seq.addOperation(op);
        return seq.getSequence();
    } catch (const runtime_error& e) {
        publishError(e.what());
        noRpcRequest = true;
        return "";
    }
}

string Parameters::processOutputMessage(string msg) {
    AlfResponseParser alfMsg(msg.c_str());
    WinCCResponse response;

    if(!alfMsg.isSuccess()) {
        returnError = true;
        return "ALF communication failed"; // provide more info about parameters
    }

    for (const auto& line : alfMsg) {
        if (line.frame.prefix == 0x000) { // response to read operation
            const vector<string>& parametersToUpdate = m_currRequestedParameterNames[line.frame.address];
            
            for (const auto& name: parametersToUpdate) {
                const ParameterInfo& parameter = m_parameterMap[name];

                double physicalValue = parameter.getPhysicalValue(line.frame.data);
                response.addParameter(name, {physicalValue});
            }
        }
    }

    return response.getContents();
}

vector<SwtSequence::SwtOperation> Parameters::handleRequest(const WinCCRequest& req) {
    m_currRequestedParameterNames.clear();
    vector<SwtSequence::SwtOperation> operations;
    for (const auto& cmd : req.getCommands()) {
        SwtSequence::SwtOperation operation = getSwtOperationForParameter(m_parameterMap[cmd.name], cmd.operation, cmd.value);
        m_currRequestedParameterNames[operation.address].push_back(cmd.name);
        operations.push_back(operation);
        if(cmd.operation != WinCCRequest::Command::Operation::Read)
            operations.push_back(getSwtOperationForParameter(m_parameterMap[cmd.name], WinCCRequest::Command::Operation::Read, std::nullopt));
    }

    return operations;
}

SwtSequence::SwtOperation Parameters::getSwtOperationForParameter(const ParameterInfo& parameter, WinCCRequest::Command::Operation operation, std::optional<double> data) {
    const std::string& parameterName = parameter.getName();
    uint32_t baseAddress = parameter.getBaseAddress();
    uint32_t regblockSize = parameter.getRegblockSize();

    if (regblockSize != 1)
        throw std::runtime_error(parameterName + ": regblock operations unsupported");

    if (operation == WinCCRequest::Command::Operation::Read)
    {
        return SwtSequence::SwtOperation(SwtSequence::Operation::Read, baseAddress, {}, true);
    }

    // WRITE operation
    if(!data.has_value())
    {
        throw std::runtime_error(parameterName + ": no data for WRITE operation");
    }
    
    uint8_t startBit = parameter.getStartBit();
    uint8_t endBit = parameter.getEndBit();
    uint8_t bitLength = parameter.getBitLength();

    if (bitLength == 32)
    {
        return SwtSequence::SwtOperation(SwtSequence::Operation::Write, baseAddress, { parameter.getRawValue(data.value()) });
    }

    // needs RMW
    std::array<uint32_t, 2> masks;
    SwtSequence::createMask(startBit, endBit, parameter.getRawValue(data.value()), masks.data());
    
    return SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, baseAddress, masks);
}