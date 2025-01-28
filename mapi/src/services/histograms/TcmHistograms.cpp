#include "services/histograms/TcmHistograms.h"
#include "TCM.h"
#include "TcmHistograms.h"
#include "utils.h"
#include <tuple>
#include <cctype>
#include <numeric>
#include <chrono>

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

    addOrReplaceHandler("RESET", [this](string) {
        return resetHistograms();
    });

    addOrReplaceHandler("COUNTER", [this](string request) {
        return handleCounterRequest(request);
    });
}

void TcmHistograms::processExecution()
{
    bool running;
    handleSleepAndWake(ReadoutInterval, running);
    if (!running) {
        return;
    }

    chrono::system_clock::time_point startTime = chrono::high_resolution_clock::now();

    vector<vector<uint32_t>> histograms = readHistograms();
    publishAnswer(parseResponse(move(histograms)));
}

bool TcmHistograms::handleCounterRequest(const string& request)
{
    auto splitResult = string_utils::Splitter::getAll(request, ',');
    if (splitResult.isError()) {
        return false;
    }

    const std::vector<std::string>& params = splitResult.ok.value();
    if (params.size() != 2 || params[0] != "COUNTER" || !std::all_of(params[1].begin(), params[1].end(), ::isdigit)) {
        return false;
    }

    uint32_t counterId = std::atoi(params[1].c_str());
    if (counterId > 15) {
        return false;
    }

    return setCounterId(counterId);
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

string TcmHistograms::parseResponse(const vector<vector<uint32_t>>&& histograms) const
{
    uint32_t wordsRead = accumulate(
        histograms.begin(),
        histograms.end(),
        0,
        [](uint32_t sum, const auto& vec) {
            return sum + vec.size();
        }
    );

    if (wordsRead * 5 + 32 > ResponseBufferSize) {
        throw runtime_error("Response may not fit inside buffer");
    }

    for (size_t i = 0; i < histograms.size(); i++) {
        
    }
}
