#include "Parameters.h"
#include "WinCCRequest.h"
#include "SwtOperation.h"
#include "AlfResponseParser.h"
#include "WinCCResponse.h"
#include <algorithm>

string Parameters::processInputMessage(string msg) {
    try {
        WinCCRequest req(msg);
        vector<SwtOperation> operations = handleRequest(req);

        // ToDo: unify SwtOperation and SwtSequence logic - what follows is a placeholder
        string seq = "";
        for (const auto& operation : operations)
            seq += operation.getRequest();
        return seq;
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
        if (line.frame.prefix == 0x000) { // read response
            const vector<string>& parametersToUpdate = m_currRequestedParameterNames[line.frame.address];
            
            for (const auto& name: parametersToUpdate) {
                const Parameter& parameter = m_parameterMap[name];

                double physicalValue = parameter.getPhysicalValue(line.frame.data);
                response.addParameter(name, {physicalValue});
            }
        }
    }

    return response.getContents();
}

vector<SwtOperation> Parameters::handleRequest(const WinCCRequest& req) {
    m_currRequestedParameterNames.clear();
    vector<SwtOperation> operations(req.getCommands().size());
    for (const auto& cmd : req.getCommands()) {
        SwtOperation operation = SwtOperation::fromParameterOperation(m_parameterMap[cmd.name], cmd.operation, cmd.value);
        m_currRequestedParameterNames[operation.address].push_back(cmd.name);
        operations.push_back(operation);
    }

    return operations;
}