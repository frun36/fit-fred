#pragma once

#include "../BasicRequestHandler.h"
#include "Fred/Mapi/indefinitemapi.h"
#include"../utils.h"

class BasicFitIndefiniteMapi: public BasicRequestHandler, public IndefiniteMapi
{
public:
    BasicFitIndefiniteMapi(std::shared_ptr<Board> board): BasicRequestHandler(board) {}
    void processExecution() = 0;

protected:

    BasicRequestHandler::ParsedResponse processSequence(BasicRequestHandler& handler, std::string request)
    {
        std::string seq;
        try {
            seq = handler.processMessageFromWinCC(request).getSequence();
        } catch (const std::exception& e) {
            return { WinCCResponse(), { { handler.getBoard()->getName(), e.what() } } };
        }
        return handler.processMessageFromALF(executeAlfSequence(seq));
    }

    BasicRequestHandler::ParsedResponse processSequence(std::string request)
    {
        return processSequence(*this, request);
    }

    template <typename T>
    std::string writeRequest(std::string_view param, T value)
    {
        return string_utils::concatenate(param, ",WRITE,", std::to_string(value));
    }

    std::string readRequest(std::string_view param)
    {
        return string_utils::concatenate(param, ",READ");
    }

    std::string& appendRequest(std::string& mess, const std::string& newRequest)
    {
        return mess.append(newRequest).append("\n");
    }
};