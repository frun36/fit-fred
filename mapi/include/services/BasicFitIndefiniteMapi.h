#pragma once

#include "../BoardCommunicationHandler.h"
#include "Fred/Mapi/indefinitemapi.h"
#include "../utils.h"

class BasicFitIndefiniteMapi : public IndefiniteMapi
{
   public:
    BasicFitIndefiniteMapi() {}

   protected:
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

    std::pair<std::vector<std::vector<uint32_t>>,BoardCommunicationHandler::ErrorReport> readFifo(BoardCommunicationHandler& handler, std::string fifoName, size_t wordsToRead){
        std::string seq;
        try {
            seq = handler.createReadFifoRequest(fifoName, wordsToRead).getSequence();
        } catch (const std::exception& e) {
            return { std::vector<std::vector<uint32_t>>(), {fifoName, e.what()}};
        }
        return {handler.parseFifo(executeAlfSequence(seq)),{}};
    }

    static const BoardCommunicationHandler::ParsedResponse EmptyResponse;
};