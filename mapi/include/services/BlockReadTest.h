#include"BasicFitIndefiniteMapi.h"

class BlockReadTest: public BasicFitIndefiniteMapi
{
    public:
    void processExecution() override
    {
        bool status = true;
        auto request = waitForRequest(status);
        if(!status){
            return;
        }
        auto data = Utility::splitString2Num(request,",");
        uint32_t address = data[0];
        uint32_t wordsNum = data[1];
        bool inc = data[3];

        auto start = std::chrono::high_resolution_clock::now();
        auto response = blockRead(address, inc, wordsNum);
        auto stop = std::chrono::high_resolution_clock::now();

        std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(stop-start);
        Print::PrintData("Block read, duration: " + std::to_string(duration.count()) + " microseconds");

        {
            auto start = std::chrono::high_resolution_clock::now();
            auto response = simpleRead(address, inc, wordsNum);
            auto stop = std::chrono::high_resolution_clock::now();

            std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(stop-start);
            Print::PrintData("Simple read, duration: " + std::to_string(duration.count()) + " microseconds");
        }

        if(response.isError()){
            publishError(response.errors.value().mess);
        }
        
        std::string responseStr = "Words: " + std::to_string(response.content.size()) + "\n";
        Print::PrintData(responseStr);
        for(auto word: response.content){
            responseStr += std::to_string(word) + "\n";
        }
       
        publishAnswer(responseStr);
    }

    BoardCommunicationHandler::BlockResponse simpleRead(uint32_t baseAddress, bool isIncrementing, uint32_t words)
    {
        SwtSequence sequence;
        for(uint32_t idx = 0; idx < words; idx++){
        switch (isIncrementing)
        {
            case true:
                sequence.addOperation(SwtSequence::Operation::Read, baseAddress, &SwtSequence::maxBlockReadSize, true);
                baseAddress += SwtSequence::maxBlockReadSize;
                break;
            default:
                sequence.addOperation(SwtSequence::Operation::Read, baseAddress, &SwtSequence::maxBlockReadSize, true);
                break;
            }
        }
    
        std::string response = executeAlfSequence(sequence.getSequence());
        AlfResponseParser parser(response);
        if (!parser.isSuccess()) {
            return { {}, BoardCommunicationHandler::ErrorReport{ "SEQUENCE", "ALF COMMUNICATION FAILED" } };
        }
    
        BoardCommunicationHandler::BlockResponse blockResponse;
        response.reserve(words);
        for(auto line: parser){
            if(line.type == AlfResponseParser::Line::Type::ResponseToWrite){
                continue;
            }
            blockResponse.content.emplace_back(line.frame.data);
        }

        return blockResponse;
    }
};