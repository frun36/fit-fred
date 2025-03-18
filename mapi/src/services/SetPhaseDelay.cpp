#include "services/SetPhaseDelay.h"
#include "board/TCM.h"
#include "utils/DelayChange.h"
#include "Alfred/print.h"

void SetPhaseDelay::processExecution()
{
    bool running = true;

    std::string request = waitForRequest(running);
    if (running == false) {
        return;
    }

    string delayReadRequest = WinCCRequest::readRequest(tcm_parameters::DelayA);
    WinCCRequest::appendToRequest(delayReadRequest, WinCCRequest::readRequest(tcm_parameters::DelayC));
    auto delayReadResponse = processSequenceThroughHandler(m_handler, delayReadRequest);
    if (delayReadResponse.isError()) {
        printAndPublishError("Couldn't read delay values");
        return;
    }

    optional<DelayChange> delayChange = DelayChange::fromWinCCRequest(m_handler, request);

    if (!delayChange.has_value()) {
        Print::PrintWarning("No delay change");
        return;
    }

    BoardCommunicationHandler::ParsedResponse parsedResponse = delayChange->apply(*this, m_handler);

    if (parsedResponse.isError()) {
        printAndPublishError(parsedResponse.getContents());
    } else {
        publishAnswer(parsedResponse.getContents());
    }
}
