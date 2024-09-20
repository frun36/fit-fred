#include "Parameters.h"
#include "WinCCRequest.h"
#include "SwtOperation.h"
#include <algorithm>

string Parameters::processInputMessage(string msg) {
    try {
        WinCCRequest req(msg);
        updateCurrOperations(req);

        // ToDo: unify SwtOperation and SwtSequence logic - what follows is a placeholder
        string seq = "";
        for (const auto& operation : m_currOperations)
            seq += operation.getRequest();
        
        return seq;
    } catch (const std::runtime_error& e) {
        publishError(e.what());
        return ""; // this probably isn't the correct way to cancel an operation
    }
}

string Parameters::processOutputMessage(string msg) {

}

void Parameters::updateCurrOperations(const WinCCRequest& req) {
    m_currOperations.clear();
    m_currOperations.resize(req.getCommands().size());
    std::transform(
        req.getCommands().begin(), 
        req.getCommands().end(), 
        m_currOperations.begin(), 
        [this](const WinCCRequest::Command& cmd) { return SwtOperation::fromParameterOperation(m_parameterMap[cmd.name], cmd.operation, cmd.value); }
    );
}
