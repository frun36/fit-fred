#pragma once

#include "../BasicRequestHandler.h"
#include "Fred/Mapi/indefinitemapi.h"
#include "../utils.h"

class BasicFitIndefiniteMapi : public BasicRequestHandler, public IndefiniteMapi
{
   public:
    BasicFitIndefiniteMapi(std::shared_ptr<Board> board) : BasicRequestHandler(board) {}

   protected:
    BasicRequestHandler::ParsedResponse processSequenceExternalHandler(BasicRequestHandler& handler, std::string request, bool raw = true)
    {
        std::string seq;
        try {
            seq = handler.processMessageFromWinCC(request, raw).getSequence();
        } catch (const std::exception& e) {
            return { WinCCResponse(), { { handler.getBoard()->getName(), e.what() } } };
        }
        return handler.processMessageFromALF(executeAlfSequence(seq));
    }

    BasicRequestHandler::ParsedResponse processSequence(std::string request)
    {
        return processSequenceExternalHandler(*this, request);
    }

    static const BasicRequestHandler::ParsedResponse EmptyResponse;
};