#pragma once

#include "services/BasicFitIndefiniteMapi.h"

using RequestHandler = std::function<bool(void)>;

class LoopingFitIndefiniteMapi : public BasicFitIndefiniteMapi
{
   private:
    bool m_stopped = false;
    std::unordered_map<std::string, RequestHandler> m_requestHandlers;

   protected:
    bool addHandler(const std::string& request, RequestHandler handler);

    struct RequestExecutionResult {
        const std::list<std::string> executed;
        const std::list<std::string> skipped;
        const std::string errorMsg;
        const bool isError;

        RequestExecutionResult(const std::list<std::string>& requests,
                               std::list<std::string>::const_iterator executedEnd,
                               bool isError = false,
                               std::string errorMsg = "")
            : executed(requests.begin(), executedEnd), skipped(executedEnd, requests.end()), isError(isError), errorMsg(errorMsg) {};

        operator std::string() const;
    };

    void handleSleepAndWake(useconds_t sleepUs, bool& running);

    RequestExecutionResult executeQueuedRequests(bool& running);

   public:
    LoopingFitIndefiniteMapi::LoopingFitIndefiniteMapi()
    {
        addHandler("START", [this]() { m_stopped = false; return true; });
        addHandler("STOP", [this]() { m_stopped = true; return true; });
    }
};