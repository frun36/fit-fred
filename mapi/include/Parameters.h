#pragma once

#include <unordered_map>
#include <string>

#include "Fred/Mapi/mapi.h"
#include "ParameterInfo.h"
#include "SwtSequence.h"
#include "WinCCRequest.h"

class Parameters : public Mapi {
public:
    Parameters(unordered_map<string, ParameterInfo> parameterMap) : m_parameterMap(parameterMap) {}

    unordered_map<string, ParameterInfo> m_parameterMap;
    unordered_map<uint32_t, vector<string>> m_currRequestedParameterNames;

    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override;

    SwtSequence::SwtOperation getSwtOperationForParameter(const ParameterInfo& parameter, WinCCRequest::Command::Operation operation, std::optional<double> data);

    vector<SwtSequence::SwtOperation> processRequest(const WinCCRequest& req);

    string generateErrorForAll(string message) const;
};