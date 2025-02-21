#pragma once

#include "services/templates/BasicFitIndefiniteMapi.h"
#include <functional>

using RequestHandler = function<Result<string, string>(vector<string>)>;

class LoopingFitIndefiniteMapi : public BasicFitIndefiniteMapi
{
   private:
    bool m_stopped;
    unordered_map<string, RequestHandler> m_requestHandlers;
    chrono::system_clock::time_point m_startTime;
    useconds_t m_elapsed = 0;

   protected:
    useconds_t getPrevElapsed() const { return m_elapsed; }
    void handleSleepAndWake(useconds_t interval, bool& running);
    bool isStopped() const { return m_stopped; }

    // Handling potential incoming requests
    struct ParsedRequest {
        const string prefix;
        const vector<string> arguments;
        const bool isMalformed;

        ParsedRequest(string prefix, vector<string> arguments, bool isMalformed = false) : prefix(std::move(prefix)), arguments(std::move(arguments)), isMalformed(isMalformed) {}
    };
    static ParsedRequest parseRequest(const string& request);

    struct RequestExecutionResult {
        const list<string> executed;
        const list<string> skipped;
        const bool isError;
        const string errorMsg;

        RequestExecutionResult(const list<string>& requests,
                               list<string>::const_iterator executedEnd,
                               bool isError = false,
                               string errorMsg = "")
            : executed(requests.begin(), executedEnd), skipped(executedEnd, requests.end()), isError(isError), errorMsg(errorMsg) {};

        operator string() const;

        bool isEmpty() const;
    };

    void addOrReplaceHandler(const string& prefix, RequestHandler handler);
    Result<string, string> executeSingleRequest(const string& req);
    RequestExecutionResult executeQueuedRequests(bool& running);
    static std::string getRequestPrefix(const std::string& request);

   public:
    LoopingFitIndefiniteMapi(bool isDefaultStopped = false);
};
