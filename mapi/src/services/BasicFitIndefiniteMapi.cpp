#include "BasicFitIndefiniteMapi.h"


BasicFitIndefiniteMapi::BasicFitIndefiniteMapi() {
    addHandler("START", [this]() { m_stopped = false; return true; });
    addHandler("STOP", [this]() { m_stopped = true; return true; });
}

BoardCommunicationHandler::ParsedResponse BasicFitIndefiniteMapi::processSequenceThroughHandler(BoardCommunicationHandler& handler, std::string request, bool readAfterWrite) {
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

BoardCommunicationHandler::FifoResponse BasicFitIndefiniteMapi::readFifo(BoardCommunicationHandler& handler, std::string fifoName, size_t wordsToRead) {
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

bool BasicFitIndefiniteMapi::addHandler(const std::string& request, RequestHandler handler) {
    return m_requestHandlers.insert({request, handler}).second;
}  

BasicFitIndefiniteMapi::RequestExecutionResult BasicFitIndefiniteMapi::executeQueuedRequests(bool& running) {
    std::list<std::string> requests;
    while (isRequestAvailable(running))
        requests.push_back(getRequest());
    
    for (auto it = requests.begin(); it != requests.end(); it++) {
        auto handlerPairIt = m_requestHandlers.find(*it);
        if (handlerPairIt == m_requestHandlers.end())
            return RequestExecutionResult(requests, it, "Request '" + *it + "' is unexpected");

        auto handler = handlerPairIt->second;
        if (!handler())
            return RequestExecutionResult(requests, it, "Execution of '" + *it + "' failed");
    }

    return RequestExecutionResult(requests, requests.end());
}
