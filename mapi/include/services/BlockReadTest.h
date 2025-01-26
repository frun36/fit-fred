#include"BasicFitIndefiniteMapi.h"

class BlockReadTest: public BasicFitIndefiniteMapi
{
    public:
    void processExecution() override
    {
        bool status = true;
        auto request = waitForRequest(status);
        auto data = Utility::splitString2Num(request,",");
        uint32_t address = data[0];
        uint32_t wordsNum = data[1];
        bool inc = data[3];

        auto response = blockRead(address, inc, wordsNum);
        if(response.isError()){
            publishError(response.errors.value().mess);
        }
        
        std::string responseStr = "Words: " + std::to_string(response.content.size()) + "\n";
        for(auto word: response.content){
            responseStr += std::to_string(word) + "\n";
        }
        Print::PrintData(responseStr);
        publishAnswer(responseStr);
    }
};