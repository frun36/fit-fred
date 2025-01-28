#include "services/histograms/TcmHistograms.h"
#include "TCM.h"
#include "utils.h"
#include <tuple>
#include <cctype>
#include <numeric>
#include <chrono>
#include <cstdio>

TcmHistograms::TcmHistograms(shared_ptr<Board> tcm) : m_handler(tcm)
{
    if (!tcm->isTcm()) {
        throw runtime_error("TcmHistograms: board is not a TCM");
    }

    size_t totalBinCount;
    for (auto name : tcm_parameters::getAllHistograms()) {
        const Board::ParameterInfo& param = tcm->at(name);
        m_histograms.emplace_back(name, param.baseAddress, param.regBlockSize);
        totalBinCount += param.regBlockSize;
    }
    sort(m_histograms.begin(), m_histograms.end());
    m_responseBufferSize = 5 * totalBinCount + 256; // 4 hex digits + comma, 256B for additional info
    m_responseBuffer = new char[m_responseBufferSize];

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

    RequestExecutionResult requestResult = executeQueuedRequests(running);
    if (requestResult.isError) {
        printAndPublishError(requestResult);
    }

    vector<vector<uint32_t>> histogramData = readHistograms();
    if (histogramData.empty()) {
        return;
    }

    publishAnswer(parseResponse(histogramData, (requestResult.isEmpty() || requestResult.isError) ? string("") : requestResult));
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
    int64_t curr = m_handler.getBoard()->at(tcm_parameters::CorrelationCountersSelect).getElectronicValueOptional().value_or(-1);
    if (curr == counterId) {
        return true;
    }

    auto parsedResponse = processSequenceThroughHandler(
        m_handler, WinCCRequest::writeRequest(tcm_parameters::CorrelationCountersSelect, counterId));

    bool result = !parsedResponse.isError();
    return result;
}

vector<vector<uint32_t>> TcmHistograms::readHistograms()
{
    if (m_histograms.size() < 2) {
        printAndPublishError("Missing histogram parameter information");
        return { {} };
    }

    int64_t selectedCounter = m_handler.getBoard()->at(tcm_parameters::CorrelationCountersSelect).getElectronicValueOptional().value_or(0);
    m_histograms[0].name = to_string(selectedCounter);
    bool selectableCounterEnabled = (selectedCounter != 0);

    uint32_t startAddress = selectableCounterEnabled ? m_histograms[0].baseAddress : m_histograms[1].baseAddress;
    uint32_t words = m_histograms.back().baseAddress + m_histograms.back().binCount - startAddress;

    auto res = blockRead(startAddress, true, words); // single read with unnecessary words is allegedly faster than separate reads

    if (res.isError()) {
        printAndPublishError(res.errors->mess);
        return { {} };
    } else {
        return parseHistogramData(res.content, startAddress);
    }
}

vector<vector<uint32_t>> TcmHistograms::parseHistogramData(const vector<uint32_t>& data, uint32_t startAddress)
{
    vector<vector<uint32_t>> result;

    for (const auto& h : m_histograms) {
        if (h.baseAddress < startAddress) {
            result.emplace_back();
            continue;
        }

        auto begin = data.begin() + (h.baseAddress - startAddress);
        auto end = begin + h.binCount;

        if (end > data.end()) {
            printAndPublishError("Incomplete histogram readout - couldn't parse all bins");
            return { {} };
        }

        result.emplace_back(begin, end);
    }

    return result;
}

const char* TcmHistograms::parseResponse(const vector<vector<uint32_t>>& histogramData, const string& requestResponse) const
{
    char* buffPos = m_responseBuffer;
    buffPos += sprintf(buffPos, "%04X\n", 32);
    for (size_t i = 0; i < m_histograms.size(); i++) {
        buffPos += sprintf(buffPos, "%s", m_histograms[i].name.c_str());
        for (uint32_t binVal : histogramData[i]) {
            buffPos += sprintf(buffPos, ",%04X", binVal);
        }
        *buffPos++ = '\n';
    }
    snprintf(buffPos, (m_responseBuffer + m_responseBufferSize) - buffPos, "%s", requestResponse.c_str()); // inserts '\0' as well, even if requestResponse is empty
    return m_responseBuffer;
}
