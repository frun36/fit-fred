#pragma once

#include "../BoardCommunicationHandler.h"

#ifdef FIT_UNIT_TEST

#include "../../../test/mocks/include/mapi.h"

#else

#include "Fred/Mapi/indefinitemapi.h"

#endif

#include "../utils.h"
#include <list>
#include <string>
#include <unordered_map>
#include <functional>
#include <sstream>

using RequestHandler = std::function<bool(void)>;

class BasicFitIndefiniteMapi : public IndefiniteMapi
{
   protected:
    bool m_stopped = false;
    std::unordered_map<std::string, RequestHandler> m_requestHandlers;

   public:
    BasicFitIndefiniteMapi();

    BoardCommunicationHandler::ParsedResponse processSequenceThroughHandler(BoardCommunicationHandler& handler, std::string request, bool readAfterWrite = true);

    BoardCommunicationHandler::FifoResponse readFifo(BoardCommunicationHandler& handler, std::string fifoName, size_t wordsToRead);

    bool addHandler(const std::string& request, RequestHandler handler);

    struct RequestExecutionResult {
        const std::list<std::string> executed;
        const std::list<std::string> skipped;
        const std::string errorMsg;
        const bool isError;

        RequestExecutionResult(const std::list<std::string>& requests,
                               std::list<std::string>::const_iterator executedEnd,
                               bool isError = false,
                               std::string errorMsg = "")
            : executed(requests.begin(), executedEnd), skipped(executedEnd, requests.end()), isError(isError), errorMsg(errorMsg) {};

        operator std::string() const;
    };

    void handleSleepAndWake(useconds_t sleepUs, bool& running);

    RequestExecutionResult executeQueuedRequests(bool& running);
};
