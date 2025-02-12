#pragma once

#include "services/LoopingFitIndefiniteMapi.h"
#include "utils.h"

class BoardHistograms : public LoopingFitIndefiniteMapi
{
   protected:
    BoardHistograms() : LoopingFitIndefiniteMapi(true)
    {
        addOrReplaceHandler("RESET", [this](vector<string>) -> Result<string, string> {
            return resetHistograms();
        });
    }

    virtual Result<string, string> resetHistograms() = 0;
    virtual bool readHistograms() = 0;
    virtual const char* parseResponse(const string& requestResultString) const = 0;

    static constexpr useconds_t ReadoutInterval = 1'000'000;
    uint32_t m_readId = 0;

    void processExecution() override
    {
        Print::PrintVerbose(name, "Entering processExecution");
        bool running;
        handleSleepAndWake(ReadoutInterval, running);
        if (!running) {
            return;
        }

        Print::PrintVerbose(name, "Executing queued requests");
        RequestExecutionResult requestResult = executeQueuedRequests(running);
        if (requestResult.isError) {
            printAndPublishError(requestResult);
        }

        Print::PrintVerbose(name, "Reading histograms");
        if (!readHistograms()) {
            return;
        }

        Print::PrintVerbose(name, "Parsing response");
        string requestResultString = (requestResult.isEmpty() || requestResult.isError) ? string("") : requestResult;
        publishAnswer(parseResponse(requestResultString));
        m_readId++;
        Print::PrintVerbose(name, "processExecution done");
    }

    virtual ~BoardHistograms() = default;
};
