#include "services/LoopingFitIndefiniteMapi.h"
#include <unistd.h>
#include <chrono>
#include <Alfred/print.h>
#include "utils.h"

LoopingFitIndefiniteMapi::LoopingFitIndefiniteMapi(bool isDefaultStopped) : m_stopped(isDefaultStopped)
{
    addOrReplaceHandler("START", [this](std::string) {
        Print::PrintInfo(name, "Service started");
        m_stopped = false;
        return true;
    });

    addOrReplaceHandler("STOP", [this](std::string) {
        Print::PrintInfo(name, "Service stopped");
        m_stopped = true;
        return true;
    });

    m_startTime = std::chrono::high_resolution_clock::now();
}

void LoopingFitIndefiniteMapi::addOrReplaceHandler(const std::string& prefix, RequestHandler handler)
{
    m_requestHandlers[prefix] = handler;
}

void LoopingFitIndefiniteMapi::handleSleepAndWake(useconds_t interval, bool& running)
{
    if (!m_stopped) {
        m_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_startTime).count();

        if (interval == 0) {
            return;
        }

        if (m_elapsed >= interval) {
            Print::PrintWarning(name, "Service overloaded: elapsed " + std::to_string(m_elapsed * 0.001) + " ms, interval " + std::to_string(interval * 0.001) + " ms");
        } else {
            usleep(interval - m_elapsed);
        }
    }

    while (m_stopped) {
        std::string request = waitForRequest(running);
        if (!running) {
            return;
        }

        std::string prefix = getRequestPrefix(request);
        if (prefix == "START") {
            m_requestHandlers["START"](request);
        } else {
            printAndPublishError("Unexpected request received while stopped: '" + request + "'");
        }
    }

    m_startTime = std::chrono::high_resolution_clock::now();
}

LoopingFitIndefiniteMapi::RequestExecutionResult LoopingFitIndefiniteMapi::executeQueuedRequests(bool& running)
{
    std::list<std::string> requests;
    while (isRequestAvailable(running)) {
        if (!running) {
            return RequestExecutionResult(requests, requests.begin(), true, "Error getting available requests: not running");
        }
        std::string currRequest = getRequest();
        // Insert only if the request is different than the last one
        // (treats consecutive identical queued requests as one)
        if (requests.empty() || requests.back() != currRequest) {
            requests.push_back(currRequest);
        }
    }

    for (std::list<std::string>::const_iterator it = requests.begin(); it != requests.end(); it++) {
        std::string prefix = getRequestPrefix(*it);
        auto handlerPairIt = m_requestHandlers.find(prefix);
        if (handlerPairIt == m_requestHandlers.end()) {
            return RequestExecutionResult(requests, it, true, "Request '" + *it + "' is unexpected");
        }

        auto handler = handlerPairIt->second;
        if (!handler(*it)) {
            return RequestExecutionResult(requests, it, true, "Execution of '" + *it + "' failed");
        }
    }

    return RequestExecutionResult(requests, requests.end());
}

LoopingFitIndefiniteMapi::RequestExecutionResult::operator std::string() const
{
    std::ostringstream oss;

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

std::string LoopingFitIndefiniteMapi::getRequestPrefix(const std::string& request) {
    string_utils::Splitter s(request, ',');
    return s.reachedEnd() ? "" : s.getNext().data();
}