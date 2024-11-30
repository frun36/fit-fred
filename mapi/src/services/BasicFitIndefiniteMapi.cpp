#include "BasicFitIndefiniteMapi.h"

BoardCommunicationHandler::ParsedResponse BasicFitIndefiniteMapi::processSequenceThroughHandler(BoardCommunicationHandler& handler, std::string request, bool readAfterWrite)
{
    if (request.size() == 0) {
        return BoardCommunicationHandler::ParsedResponse::EmptyResponse;
    }
    std::string seq;
    try {
        seq = handler.processMessageFromWinCC(request, readAfterWrite).getSequence();
    } catch (const std::exception& e) {
        return { WinCCResponse(), { { handler.getBoard()->getName(), e.what() } } };
    }
    return handler.processMessageFromALF(executeAlfSequence(seq));
}

BoardCommunicationHandler::FifoResponse BasicFitIndefiniteMapi::readFifo(BoardCommunicationHandler& handler, std::string fifoName, size_t wordsToRead)
{
    if (wordsToRead == 0)
        return BoardCommunicationHandler::FifoResponse::EmptyFifoResponse;

    std::string seq;
    try {
        seq = handler.createReadFifoRequest(fifoName, wordsToRead).getSequence();
    } catch (const std::exception& e) {
        return { std::vector<std::vector<uint32_t>>(), BoardCommunicationHandler::ErrorReport{ fifoName, e.what() } };
    }
    return handler.parseFifo(executeAlfSequence(seq));
}
