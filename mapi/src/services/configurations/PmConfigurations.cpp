#include "services/configurations/PmConfigurations.h"
#include "Alfred/print.h"

PmConfigurations::PmConfigurations(std::shared_ptr<Board> board) : BoardConfigurations(board) {}

string PmConfigurations::processInputMessage(string request)
{
    noRpcRequest = false;
    m_handler.getBoard()->setEnvironment(getServiceName(),1);
    std::string sequence;
    
    try{
        const string& configurationName = request;
        m_configurationInfo = getConfigurationInfo(configurationName);
        const string& req = m_configurationInfo.req;
        Print::PrintVerbose("Configuration '" + name + "' for " + m_boardName + ":\n" + req);
        sequence = m_handler.processMessageFromWinCC(req).getSequence();
    }
    catch(const std::exception& e){
        m_handler.getBoard()->setEnvironment(getServiceName(),0);
        publishError(string_utils::concatenate("Excpetion: ", e.what()));
        noRpcRequest = true;
        return {};
    }
    
    return sequence;
}

string PmConfigurations::processOutputMessage(string msg)
{
    auto parsedResponse = m_handler.processMessageFromALF(msg);
    if (parsedResponse.isError()) {
        returnError = true;
    } else {
        Print::PrintInfo("Configuration '" + m_configurationInfo.name + "' successfully applied to " + m_boardName);
    }

    m_handler.getBoard()->setEnvironment(getServiceName(),0);
    return parsedResponse.getContents();
}
