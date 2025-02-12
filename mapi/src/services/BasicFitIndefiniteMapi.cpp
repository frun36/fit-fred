#include "services/BasicFitIndefiniteMapi.h"

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
    if (wordsToRead == 0) {
        return BoardCommunicationHandler::FifoResponse::EmptyFifoResponse;
    }

    std::string seq;
    try {
        seq = handler.createReadFifoRequest(fifoName, wordsToRead).getSequence();
    } catch (const std::exception& e) {
        return { std::vector<std::vector<uint32_t>>(), BoardCommunicationHandler::ErrorReport{ fifoName, e.what() } };
    }
    return handler.parseFifo(executeAlfSequence(seq));
}


BoardCommunicationHandler::BlockResponse BasicFitIndefiniteMapi::blockRead(uint32_t baseAddress, bool isIncrementing, uint32_t words)
{
    uint32_t fullPacketsNumber = words / SwtSequence::maxBlockReadSize;
    uint32_t offset = words % SwtSequence::maxBlockReadSize;

    SwtSequence sequence;

    if(offset < words){
        for(uint32_t idx = 0; idx < fullPacketsNumber; idx++){
        switch (isIncrementing)
        {
            case true:
                sequence.addOperation(SwtSequence::Operation::BlockRead, baseAddress, &SwtSequence::maxBlockReadSize, true);
                baseAddress += SwtSequence::maxBlockReadSize;
                break;
            default:
                sequence.addOperation(SwtSequence::Operation::BlockReadNonIncrement, baseAddress, &SwtSequence::maxBlockReadSize, true);
                break;
            }
        }
    }

    if(isIncrementing)
    {
        sequence.addOperation(SwtSequence::Operation::BlockRead, baseAddress, &offset, true);
    }else{
        sequence.addOperation(SwtSequence::Operation::BlockReadNonIncrement, baseAddress, &offset, true);
    }

    std::string response = executeAlfSequence(sequence.getSequence());
    AlfResponseParser parser(response);
    if (!parser.isSuccess()) {
        return { {}, BoardCommunicationHandler::ErrorReport{ "SEQUENCE", "ALF COMMUNICATION FAILED" } };
    }
    
    BoardCommunicationHandler::BlockResponse blockResponse;
    blockResponse.content.resize(words); // much faster than using reserve + emplace_back
    uint32_t idx = 0;
    for(auto line: parser){
        if(line.type == AlfResponseParser::Line::Type::ResponseToWrite){
            continue;
        }
        if(idx >= words){
            return {{}, BoardCommunicationHandler::ErrorReport{ "SEQUENCE", "Received response contains too many SWT frames" }};
        }
        blockResponse.content[idx++] = line.frame.data;
    }
    if(idx < words){
        return {{}, BoardCommunicationHandler::ErrorReport{"SEQUENCE", "Received incomplete response; received " + std::to_string(idx)  + " words, expected " + std::to_string(words)}};
    }

    return blockResponse;
}