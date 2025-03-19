#include "services/SetPhaseDelay.h"
#include "utils/DelayChange.h"
#include "Alfred/print.h"

void SetPhaseDelay::processExecution()
{
    bool running = true;

    std::string request = waitForRequest(running);
    if (running == false) {
        return;
    }

    optional<DelayChange> delayChange = DelayChange::fromWinCCRequest(m_handler.getBoard(), request);

    if (!delayChange.has_value()) {
        Print::PrintWarning("No delay change");
        return;
    }

    BoardCommunicationHandler::ParsedResponse parsedResponse = delayChange->apply(*this, m_handler);

    if (parsedResponse.isError()) {
        publishError(parsedResponse.getContents());
    } else {
        publishAnswer(parsedResponse.getContents());
    }
}
