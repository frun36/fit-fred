#include "services/LoopingFitIndefiniteMapi.h"
#include <unistd.h>
#include <chrono>
#include <Alfred/print.h>

LoopingFitIndefiniteMapi::LoopingFitIndefiniteMapi(bool isDefaultStopped) : m_stopped(isDefaultStopped)
{
    addHandler("START", [this]() {
        Print::PrintInfo(name, "Service started");
        m_stopped = false;
        return true;
    });

    addHandler("STOP", [this]() {
        Print::PrintInfo(name, "Service stopped");
        m_stopped = true;
        return true;
    });

    m_startTime = std::chrono::high_resolution_clock::now();
}

bool LoopingFitIndefiniteMapi::addHandler(const std::string& request, RequestHandler handler)
{
    return m_requestHandlers.insert({ request, handler }).second;
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

        if (request == "START") {
            m_requestHandlers["START"]();
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
        requests.push_back(getRequest());
    }

    for (std::list<std::string>::const_iterator it = requests.begin(); it != requests.end(); it++) {
        auto handlerPairIt = m_requestHandlers.find(*it);
        if (handlerPairIt == m_requestHandlers.end()) {
            return RequestExecutionResult(requests, it, true, "Request '" + *it + "' is unexpected");
        }

        auto handler = handlerPairIt->second;
        if (!handler()) {
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
