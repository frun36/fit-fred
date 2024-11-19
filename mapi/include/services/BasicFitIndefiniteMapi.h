#pragma once

#include "../BoardCommunicationHandler.h"

#ifdef FIT_UNIT_TEST

#include "../../../test/mocks/include/mapi.h"

#else

#include "Fred/Mapi/indefinitemapi.h"

#endif

#include "../utils.h"
#include "DelayChange.h"

class BasicFitIndefiniteMapi : public IndefiniteMapi
{
   public:
    BasicFitIndefiniteMapi() {}

    BoardCommunicationHandler::ParsedResponse processSequenceThroughHandler(BoardCommunicationHandler& handler, std::string request, bool readAfterWrite = true)
    {
        std::string seq;
        try {
            seq = handler.processMessageFromWinCC(request, readAfterWrite).getSequence();
        } catch (const std::exception& e) {
            return { WinCCResponse(), { { handler.getBoard()->getName(), e.what() } } };
        }
        return handler.processMessageFromALF(executeAlfSequence(seq));
    }

    BoardCommunicationHandler::FifoResponse readFifo(BoardCommunicationHandler& handler, std::string fifoName, size_t wordsToRead){
        std::string seq;
        try {
            seq = handler.createReadFifoRequest(fifoName, wordsToRead).getSequence();
        } catch (const std::exception& e) {
            return { std::vector<std::vector<uint32_t>>(), BoardCommunicationHandler::ErrorReport{fifoName, e.what()}};
        }
        return handler.parseFifo(executeAlfSequence(seq));
    }

    static const BoardCommunicationHandler::ParsedResponse EmptyResponse;
};