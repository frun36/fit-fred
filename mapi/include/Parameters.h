#pragma once

#include <unordered_map>
#include <string>

#include "Fred/Mapi/mapi.h"
#include "Parameter.h"
#include "SwtOperation.h"
#include "WinCCRequest.h"

class Parameters : public Mapi {
    std::unordered_map<std::string, Parameter> m_parameterMap;
    std::vector<SwtOperation> m_currOperations; 

    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override;

    void updateCurrOperations(const WinCCRequest& req);
};