#include "services/Parameters.h"
#include <algorithm>

string Parameters::processInputMessage(string msg)
{
    returnError = false;
    try {
        SwtSequence sequence = processMessageFromWinCC(msg);
        return sequence.getSequence();
    } catch (const std::exception& e) {
        throw;
    }
}

string Parameters::processOutputMessage(string msg)
{
    auto parsedResponse = processMessageFromALF(msg);
    if (parsedResponse.isError())
        returnError = true;
    return parsedResponse.getContents();
}
