#pragma once

#include "services/BasicFitIndefiniteMapi.h"

using RequestHandler = std::function<bool(std::string)>;

class LoopingFitIndefiniteMapi : public BasicFitIndefiniteMapi
{
   private:
    bool m_stopped;
    std::unordered_map<std::string, RequestHandler> m_requestHandlers;
    chrono::system_clock::time_point m_startTime;
    useconds_t m_elapsed = 0;

   protected:
    useconds_t getPrevElapsed() const { return m_elapsed; }
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

        bool isEmpty() const;
    };

    void addOrReplaceHandler(const std::string& prefix, RequestHandler handler);
    RequestExecutionResult executeQueuedRequests(bool& running);
    static std::string getRequestPrefix(const std::string& request);

   public:
    LoopingFitIndefiniteMapi(bool isDefaultStopped = false);
};