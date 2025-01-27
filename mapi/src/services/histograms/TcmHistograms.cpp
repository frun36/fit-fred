#include "services/histograms/TcmHistograms.h"
#include "TCM.h"
#include "TcmHistograms.h"
#include "utils.h"
#include <tuple>
#include <cctype>
#include <numeric>

TcmHistograms::TcmHistograms(shared_ptr<Board> tcm) : m_handler(tcm), m_baseParam(tcm->at(tcm_parameters::HistogramsAll))
{
    if (!tcm->isTcm()) {
        throw runtime_error("TcmHistograms: board is not a TCM");
    }

    for (auto name : tcm_parameters::getAllHistograms()) {
        const Board::ParameterInfo& param = tcm->at(name);
        if (param.baseAddress + param.regBlockSize > m_baseParam.baseAddress + m_baseParam.regBlockSize) {
            throw runtime_error("Invalid TCM histogram parameters - param " + param.name + " doesn't fit in base");
        }
        m_histograms.emplace(name, param.baseAddress - m_baseParam.baseAddress, param.regBlockSize);
    }
}

void TcmHistograms::processExecution()
{
    bool running;
    string request;
    if (!m_doReadout) {
        request = waitForRequest(running);
    } else {
        if (isRequestAvailable(running)) {
            request = getRequest();
        }
    }

    if (!request.empty()) {
        handleRequest(request);
    }

    if (!m_doReadout) {
        return;
    }

    chrono::system_clock::time_point startTime = chrono::high_resolution_clock::now();

    vector<vector<uint32_t>> histograms = readHistograms();
    uint32_t wordsRead = accumulate(
        histograms.begin(),
        histograms.end(),
        0,
        [](uint32_t sum, const auto& vec) {
            return sum + vec.size();
        }
    );

    string response;
    response.reserve(wordsRead * 9 + 64); // 8 hex chars, comma, some additional space for headers/names

}

void TcmHistograms::handleRequest(const string& request)
{
    if (request == "STOP") {
        if (stopHistogramming()) {
            return;
        } else {
            throw runtime_error("Failed to stop histogramming");
        }
    } else if (request == "RESET") {
        if (resetHistograms()) {
            return;
        } else {
            throw runtime_error("Failed to reset histograms");
        }
    }

    auto splitResult = string_utils::Splitter::getAll(request, ',');
    if (splitResult.isError()) {
        throw runtime_error("Failed to parse message");
    }

    const std::vector<std::string>& params = splitResult.ok.value();
    if (params.size() != 2 || params[0] != "READ" || !std::all_of(params[1].begin(), params[1].end(), ::isdigit)) {
        throw runtime_error("Read command must be two comma separated values - READ,[counter_id]");
    }

    uint32_t counterId = std::atoi(params[1].c_str());
    if (counterId > 15) {
        throw runtime_error("Counter ID must be either 0 (none) or between 1 and 15");
    }

    if (!(setCounterId(counterId) && startHistogramming())) {
        throw runtime_error("Failed to update counter ID and start histogramming");
    }
}

bool TcmHistograms::startHistogramming()
{
    m_doReadout = true;
    return true;
}

bool TcmHistograms::stopHistogramming()
{
    m_doReadout = false;
    return true;
}

bool TcmHistograms::resetHistograms()
{
    auto parsedResponse = processSequenceThroughHandler(
        m_handler, WinCCRequest::writeRequest(tcm_parameters::ResetHistograms, 1), false);
    return !parsedResponse.isError();
}

bool TcmHistograms::setCounterId(uint32_t counterId)
{
    int64_t curr = m_handler.getBoard()->at(tcm_parameters::CorrelationCounterSelect).getElectronicValueOptional().value_or(-1);
    if (curr == counterId) {
        return true;
    }

    auto parsedResponse = processSequenceThroughHandler(
        m_handler, WinCCRequest::writeRequest(tcm_parameters::CorrelationCounterSelect, counterId));
    return !parsedResponse.isError();
}

vector<vector<uint32_t>> TcmHistograms::readHistograms() {
    // todo
    return {{}};
}

string TcmHistograms::parseResponse(vector<vector<uint32_t>> data) const
{
    return string();
}
