#include "Parameters.h"
#include "WinCCRequest.h"
#include "SwtSequence.h"
#include "AlfResponseParser.h"
#include "WinCCResponse.h"
#include <algorithm>

string Parameters::processInputMessage(string msg) {
    returnError = false;
    try {
        SwtSequence sequence = processMessageFromWinCC(msg);
        return sequence.getSequence();
    } catch (const std::exception& e) {
        throw;
    }
}

string Parameters::processOutputMessage(string msg) {
    auto parsedResponse = processMessageFromALF(msg);
    if(parsedResponse.errors.size() != 0)
    {
        returnError = true;
        std::stringstream error;
        for(auto& report: parsedResponse.errors)
        {
            error << report.what() << '\n';
        }
        error << parsedResponse.response.getContents();
        return error.str();
    }
    return parsedResponse.response.getContents();
}

