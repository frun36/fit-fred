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
    } catch (const runtime_error& e) {
        throw;
    }
}

string Parameters::processOutputMessage(string msg) {
    auto parsedResponse = processMessageFromALF(msg);
    if(parsedResponse.second.size() != 0)
    {
        returnError = true;
        std::stringstream error;
        for(auto& report: parsedResponse.second)
        {
            error << report.what() << '\n';
        }
        error << parsedResponse.first.getContents();
        return error.str();
    }
    return parsedResponse.first.getContents();
}

