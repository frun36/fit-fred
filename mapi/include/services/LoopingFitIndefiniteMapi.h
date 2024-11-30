#pragma once

#include "services/BasicFitIndefiniteMapi.h"

using RequestHandler = std::function<bool(void)>;

class LoopingFitIndefiniteMapi : public BasicFitIndefiniteMapi
{
   private:
    bool m_stopped = false;
    std::unordered_map<std::string, RequestHandler> m_requestHandlers;
    chrono::system_clock::time_point m_startTime;
    useconds_t m_elapsed = 0;

   protected:
    useconds_t getElapsed() const { return m_elapsed; }
    void handleSleepAndWake(useconds_t interval, bool& running);
    
    // Handling potential incoming requests
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

    bool addHandler(const std::string& request, RequestHandler handler);
    RequestExecutionResult executeQueuedRequests(bool& running);

   public:
    LoopingFitIndefiniteMapi();
};