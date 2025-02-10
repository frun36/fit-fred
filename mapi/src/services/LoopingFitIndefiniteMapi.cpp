#include "services/LoopingFitIndefiniteMapi.h"
#include <unistd.h>
#include <chrono>
#include <Alfred/print.h>
#include "utils.h"

LoopingFitIndefiniteMapi::LoopingFitIndefiniteMapi(bool isDefaultStopped) : m_stopped(isDefaultStopped)
{
    addOrReplaceHandler("START", [this](vector<string>) -> Result<string, string> {
        m_stopped = false;
        return { .ok = "Service started", .error = nullopt };
    });

    addOrReplaceHandler("STOP", [this](vector<string>) -> Result<string, string> {
        m_stopped = true;
        return { .ok = "Service stopped", .error = nullopt };
    });

    m_startTime = chrono::high_resolution_clock::now();
}

void LoopingFitIndefiniteMapi::addOrReplaceHandler(const string& prefix, RequestHandler handler)
{
    m_requestHandlers[prefix] = handler;
}

void LoopingFitIndefiniteMapi::handleSleepAndWake(useconds_t interval, bool& running)
{
    if (!m_stopped) {
        m_elapsed = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - m_startTime).count();

        if (interval == 0) {
            return;
        }

        if (m_elapsed >= interval) {
            Print::PrintWarning(name, "Service overloaded: elapsed " + to_string(m_elapsed * 0.001) + " ms, interval " + to_string(interval * 0.001) + " ms");
        } else {
            usleep(interval - m_elapsed);
        }
    }

    while (m_stopped) {
        string request = waitForRequest(running);
        if (!running) {
            return;
        }

        auto result = executeSingleRequest(request);
        if (result.isOk()) {
            Print::PrintInfo(name, *result.ok);
        } else {
            printAndPublishError("Failed to execute " + request + ": " + *result.error);
        }
    }

    m_startTime = chrono::high_resolution_clock::now();
}

Result<string, string> LoopingFitIndefiniteMapi::executeSingleRequest(const string& req)
{
    ParsedRequest pr = parseRequest(req);
    if (pr.isMalformed) {
        return { .ok = nullopt, .error = "Request " + req + " is malformed: expected [PREFIX][,OPTIONAL,ARGUMENTS,...]" };
    }

    auto handlerPairIt = m_requestHandlers.find(pr.prefix);
    if (handlerPairIt == m_requestHandlers.end()) {
        return { .ok = nullopt, .error = "Request " + pr.prefix + " is unexpected" };
    }

    auto handler = handlerPairIt->second;
    return handler(pr.arguments);
}
LoopingFitIndefiniteMapi::RequestExecutionResult LoopingFitIndefiniteMapi::executeQueuedRequests(bool& running)
{
    list<string> requests;
    while (isRequestAvailable(running)) {
        if (!running) {
            return RequestExecutionResult(requests, requests.begin(), true, "Error getting available requests: not running");
        }
        string currRequest = getRequest();
        // Insert only if the request is different than the last one
        // (treats consecutive identical queued requests as one)
        if (requests.empty() || requests.back() != currRequest) {
            requests.push_back(currRequest);
        }
    }

    for (list<string>::const_iterator it = requests.begin(); it != requests.end(); it++) {
        auto result = executeSingleRequest(*it);
        if (!result.isOk()) {
            return RequestExecutionResult(requests, it, true, *result.error);
        }
    }

    return RequestExecutionResult(requests, requests.end());
}

LoopingFitIndefiniteMapi::RequestExecutionResult::operator string() const
{
    ostringstream oss;

    oss << "Executed: ";
    for (const auto& req : executed) {
        oss << req << "; ";
    }
    if (!isError) {
        return oss.str();
    }

    oss << '\n';
    oss << "Skipped: ";
    for (const auto& req : skipped) {
        oss << req << "; ";
    }
    oss << "\nError: " << errorMsg;
    return oss.str();
}

bool LoopingFitIndefiniteMapi::RequestExecutionResult::isEmpty() const
{
    return executed.empty() && skipped.empty();
}

LoopingFitIndefiniteMapi::ParsedRequest LoopingFitIndefiniteMapi::parseRequest(const string& request)
{
    auto result = string_utils::Splitter::getAll(request, ',');
    if (!result.isOk()) {
        return ParsedRequest("", {}, true);
    }
    if (result.ok->empty()) {
        return ParsedRequest("", {}, true);
    }
    string prefix = result.ok->front();
    result.ok->erase(result.ok->begin());

    return ParsedRequest(prefix, *result.ok);
}
