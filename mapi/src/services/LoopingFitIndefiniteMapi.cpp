#include "services/LoopingFitIndefiniteMapi.h"
#include <unistd.h>

#include <unistd.h>

bool LoopingFitIndefiniteMapi::addHandler(const std::string& request, RequestHandler handler)
{
    return m_requestHandlers.insert({ request, handler }).second;
}

void LoopingFitIndefiniteMapi::handleSleepAndWake(useconds_t sleepUs, bool& running)
{
    if (!m_stopped)
        usleep(sleepUs);

    while (m_stopped) {
        std::string request = waitForRequest(running);
        if (!running)
            return;

        if (request == "START")
            m_stopped = false;
        else
            publishError("Unexpected request received while stopped: '" + request + "'");
    }
}

LoopingFitIndefiniteMapi::RequestExecutionResult LoopingFitIndefiniteMapi::executeQueuedRequests(bool& running)
{
    std::list<std::string> requests;
    while (isRequestAvailable(running))
        requests.push_back(getRequest());

    for (std::list<std::string>::const_iterator it = requests.begin(); it != requests.end(); it++) {
        auto handlerPairIt = m_requestHandlers.find(*it);
        if (handlerPairIt == m_requestHandlers.end())
            return RequestExecutionResult(requests, it, "Request '" + *it + "' is unexpected");

        auto handler = handlerPairIt->second;
        if (!handler())
            return RequestExecutionResult(requests, it, "Execution of '" + *it + "' failed");
    }

    return RequestExecutionResult(requests, requests.end());
}

LoopingFitIndefiniteMapi::RequestExecutionResult::operator std::string() const
{
    std::ostringstream oss;

    oss << "Executed: ";
    for (const auto& req : executed)
        oss << req << '; ';
    if (!isError)
        return oss.str();

    oss << '\n';
    oss << "Skipped: ";
    for (const auto& req : skipped)
        oss << req << '; ';
    oss << "\nError: " << errorMsg;
    return oss.str();
}