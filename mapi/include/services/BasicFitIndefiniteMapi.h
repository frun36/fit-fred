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
#include "Alfred/print.h"

class BasicFitIndefiniteMapi : public IndefiniteMapi
{
   public:
    BoardCommunicationHandler::ParsedResponse processSequenceThroughHandler(BoardCommunicationHandler& handler, std::string request, bool readAfterWrite = true);
    BoardCommunicationHandler::FifoResponse readFifo(BoardCommunicationHandler& handler, std::string fifoName, size_t wordsToRead);
    BoardCommunicationHandler::BlockResponse blockRead(uint32_t baseAddress, bool isIncrementing, uint32_t words);

    // Not to be used in constructors, due to the use of `name` in `PrintError`
    inline void printAndPublishError(const string& errorMsg) {
        Print::PrintError(name, errorMsg);
        publishError(errorMsg);
    }

    inline void printAndPublishError(const BoardCommunicationHandler::ParsedResponse& response) {
        Print::PrintError(name, response.getError());
        publishError(response.getContents());
    }
};
