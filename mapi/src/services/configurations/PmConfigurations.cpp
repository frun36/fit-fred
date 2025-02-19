#include "services/configurations/PmConfigurations.h"
#include "Alfred/print.h"

PmConfigurations::PmConfigurations(std::shared_ptr<Board> board) : BoardConfigurations(board) {}

string PmConfigurations::processInputMessage(string request)
{
    const string& configurationName = request;
    m_configurationInfo = getConfigurationInfo(configurationName);
    const string& req = m_configurationInfo.req;
    Print::PrintVerbose("Configuration '" + name + "' for " + m_boardName + ":\n" + req);
    return m_handler.processMessageFromWinCC(req).getSequence();
}

string PmConfigurations::processOutputMessage(string msg)
{
    auto parsedResponse = m_handler.processMessageFromALF(msg);
    if (parsedResponse.isError()) {
        returnError = true;
    } else {
        Print::PrintInfo("Configuration '" + m_configurationInfo.name + "' successfully applied to " + m_boardName);
    }
    return parsedResponse.getContents();
}
