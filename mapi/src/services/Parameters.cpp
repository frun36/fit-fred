#include "services/Parameters.h"
#include <algorithm>

string Parameters::processInputMessage(string msg)
{
    returnError = false;
    try {
        SwtSequence sequence = m_boardHandler.processMessageFromWinCC(msg);
        return sequence.getSequence();
    } catch (const std::exception& e) {
        throw;
    }
}

string Parameters::processOutputMessage(string msg)
{
    auto parsedResponse = m_boardHandler.processMessageFromALF(msg);
    if (parsedResponse.isError())
        returnError = true;
    return parsedResponse.getContents();
}
