#pragma once

#include <unordered_map>
#include <string>

#include "Fred/Mapi/mapi.h"
#include "Parameter.h"
#include "SwtSequence.h"
#include "WinCCRequest.h"

class Parameters : public Mapi {
public:
    Parameters(unordered_map<string, Parameter> parameterMap) : m_parameterMap(parameterMap) {}

    unordered_map<string, Parameter> m_parameterMap;
    unordered_map<uint32_t, vector<string>> m_currRequestedParameterNames;

    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override;

    SwtSequence::SwtOperation getSwtOperationForParameter(const Parameter& parameter, WinCCRequest::Command::Operation operation, std::optional<double> data);

    vector<SwtSequence::SwtOperation> handleRequest(const WinCCRequest& req);
};