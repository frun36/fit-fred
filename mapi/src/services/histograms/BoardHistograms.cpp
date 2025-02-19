#include "services/histograms/BoardHistograms.h"

BoardHistograms::BoardHistograms() : LoopingFitIndefiniteMapi(true)
{
    addOrReplaceHandler("RESET", [this](vector<string>) -> Result<string, string> {
        return resetHistograms();
    });

    addOrReplaceHandler("READ", [this](vector<string>) -> Result<string, string> {
        if (!isStopped()) {
            return { .ok = nullopt, .error = "Additional read can only be performed when the service is stopped" };
        }

        auto readResult = handleReadout();

        if (readResult.isOk()) {
            handleResponse(readResult.ok->count());
            return { .ok = "", .error = nullopt };
        } else {
            return { .ok = nullopt, .error = readResult.error };
        }
    });
}

Result<chrono::duration<double, milli>, string> BoardHistograms::handleReadout()
{
    auto readStart = chrono::system_clock::now();

    Result<string, string> readResult = readAndStoreHistograms();

    auto readEnd = chrono::system_clock::now();
    chrono::duration<double, milli> readElapsed = readEnd - readStart;

    if (readResult.isOk()) {
        m_readId++;
        return { .ok = readElapsed, .error = nullopt };
    } else {
        return { .ok = nullopt, .error = readResult.error };
    }
}

void BoardHistograms::handleResponse(double readElapsed, string requestResultString)
{
    m_responseOss.str("");
    m_responseOss.clear();
    m_responseOss << m_readId << "\n";
    parseResponse(m_responseOss);
    m_responseOss << "READ_ELAPSED," << readElapsed << "ms\n"
                  << "PREV_ELAPSED," << getPrevElapsed() * 1e-3 << "ms\n"
                  << requestResultString.c_str();
    publishAnswer(m_responseOss.str());
}

void BoardHistograms::processExecution()
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
    auto readResult = handleReadout();
    if (readResult.isError()) {
        printAndPublishError(*readResult.error);
        return;
    }

    Print::PrintData("Parsing response");
    double readElapsed = readResult.ok->count();
    string requestResultString = (requestResult.isEmpty() || requestResult.isError) ? string("") : requestResult;
    handleResponse(readElapsed, requestResultString);

    Print::PrintData("processExecution done");
}
