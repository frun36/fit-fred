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

class BasicFitIndefiniteMapi : public IndefiniteMapi
{
   public:
    BoardCommunicationHandler::ParsedResponse processSequenceThroughHandler(BoardCommunicationHandler& handler, std::string request, bool readAfterWrite = true);
    BoardCommunicationHandler::FifoResponse readFifo(BoardCommunicationHandler& handler, std::string fifoName, size_t wordsToRead);
};
