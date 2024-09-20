#pragma once

#include <unordered_map>
#include <string>

#include "Fred/Mapi/mapi.h"
#include "Parameter.h"
#include "SwtOperation.h"
#include "WinCCRequest.h"

class Parameters : public Mapi {
    unordered_map<string, Parameter> m_parameterMap;
    unordered_map<uint32_t, vector<string>> m_currRequestedParameterNames;

    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override;

    vector<SwtOperation> handleRequest(const WinCCRequest& req);
};