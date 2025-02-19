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

        addOrReplaceHandler("READ", [this](vector<string>) -> Result<string, string> {
            if (!isStopped()) {
                return { .ok = nullopt, .error = "Additional read can only be performed when the service is stopped" };
            }

            Result<string, string> readResult = readAndStoreHistograms();
            if (readResult.isOk()) {
                publishAnswer(parseResponse(""));
            }
            return readResult;
        });
    }

    virtual Result<string, string> resetHistograms() = 0;
    virtual Result<string, string> readAndStoreHistograms() = 0;
    virtual string parseResponse(const string& requestResultString) const = 0;

    static constexpr useconds_t ReadoutInterval = 1'000'000;
    uint32_t m_readId = 0;

    void processExecution() override
    {
        Print::PrintData("Entering processExecution");
        bool running;
        handleSleepAndWake(ReadoutInterval, running);
        if (!running) {
            return;
        }

        Print::PrintData("Executing queued requests");
        RequestExecutionResult requestResult = executeQueuedRequests(running);
        if (requestResult.isError) {
            printAndPublishError(requestResult);
        }

        Print::PrintData("Reading histograms");
        Result<string, string> readResult = readAndStoreHistograms();
        if (readResult.isError()) {
            printAndPublishError(*readResult.error);
            return;
        }

        Print::PrintData("Parsing response");
        string requestResultString = (requestResult.isEmpty() || requestResult.isError) ? string("") : requestResult;
        publishAnswer(parseResponse(requestResultString));
        m_readId++;
        Print::PrintData("processExecution done");
    }

    virtual ~BoardHistograms() = default;
};
