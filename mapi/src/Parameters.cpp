#include "Parameters.h"
#include "WinCCRequest.h"
#include "SwtSequence.h"
#include "AlfResponseParser.h"
#include "WinCCResponse.h"
#include <algorithm>

string Parameters::processInputMessage(string msg) {
    try {
        WinCCRequest req(msg);
        vector<SwtSequence::SwtOperation> operations = processRequest(req);
        SwtSequence seq(operations);
        return seq.getSequence();
    } catch (const runtime_error& e) {
        throw;
    }
}

string Parameters::processOutputMessage(string msg) {
    try {
        AlfResponseParser alfMsg(msg);

        if(!alfMsg.isSuccess()) {
            returnError = true;
            return generateErrorForAll("ALF communication failed"); // provide more info about parameters
        }
        
        WinCCResponse response;

        for (const auto& line : alfMsg) {
            if (line.frame.prefix == 0x000) { // response to read operation
                const vector<string>& parametersToUpdate = m_currRequestedParameterNames[line.frame.address];
                
                for (const auto& name: parametersToUpdate)
                    response.addParameter(name, {m_parameterMap[name].calculatePhysicalValue(line.frame.data)});
            }
        }

        return response.getContents();

    } catch (const runtime_error& e) {
        returnError = true;
        return generateErrorForAll("Invalid response from ALF: " + string(e.what()));
    }
}

vector<SwtSequence::SwtOperation> Parameters::processRequest(const WinCCRequest& req) {
    m_currRequestedParameterNames.clear();
    vector<SwtSequence::SwtOperation> operations;
    for (const auto& cmd : req.getCommands()) {
        // Convert to SWT, adding a read after every write
        SwtSequence::SwtOperation operation = getSwtOperationForParameter(m_parameterMap[cmd.name], cmd.operation, cmd.value);
        operations.push_back(operation);
        if(cmd.operation != WinCCRequest::Operation::Read)
            operations.push_back(getSwtOperationForParameter(m_parameterMap[cmd.name], WinCCRequest::Operation::Read, std::nullopt));
        
        // Store requested parameter name
        m_currRequestedParameterNames[operation.address].push_back(cmd.name);
    }

    return operations;
}

SwtSequence::SwtOperation Parameters::getSwtOperationForParameter(const ParameterInfo& parameter, WinCCRequest::Operation operation, std::optional<double> data) {
    const std::string& parameterName = parameter.name;
    uint32_t baseAddress = parameter.baseAddress;

    if (parameter.regBlockSize != 1)
    {
        throw std::runtime_error(parameterName + ": regblock operations unsupported");
    }

    if (operation == WinCCRequest::Operation::Read)
    {
        return SwtSequence::SwtOperation(SwtSequence::Operation::Read, baseAddress, {}, true);
    }

    // WRITE operation
    if(parameter.isReadonly)
        throw std::runtime_error(parameterName + ": attempted WRITE on read-only parameter");

    if(!data.has_value())
        throw std::runtime_error(parameterName + ": no data for WRITE operation");

    if (parameter.bitLength == 32)
        return SwtSequence::SwtOperation(SwtSequence::Operation::Write, baseAddress, { parameter.calculateRawValue(data.value()) });

    // needs RMW
    std::array<uint32_t, 2> masks;
    SwtSequence::createMask(parameter.startBit, parameter.bitLength, parameter.calculateRawValue(data.value()) >> parameter.startBit, masks.data());
    return SwtSequence::SwtOperation(SwtSequence::Operation::RMWbits, baseAddress, masks);
}

string Parameters::generateErrorForAll(string message) const {
    string result = "";
    for (const auto& pair : m_currRequestedParameterNames)
        for (const std::string& name : pair.second)
            result += name + ": " + message + "\n";
    return result;
}