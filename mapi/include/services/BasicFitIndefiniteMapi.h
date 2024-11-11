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

    static const BoardCommunicationHandler::ParsedResponse EmptyResponse;
};