#include "services/histograms/TcmHistograms.h"
#include "TCM.h"
#include "utils.h"
#include <cctype>
#include <cstdio>
#include <sstream>
#include <string>
#include <cinttypes>

TcmHistograms::TcmHistograms(shared_ptr<Board> tcm) : m_handler(tcm)
{
    if (!tcm->isTcm()) {
        throw runtime_error("TcmHistograms: board is not a TCM");
    }

    size_t totalBinCount = 0;
    for (auto name : tcm_parameters::getAllHistograms()) {
        const Board::ParameterInfo& param = tcm->at(name);
        m_histograms.emplace_back(string(name), param.baseAddress, param.regBlockSize);
        totalBinCount += param.regBlockSize;
    }
    sort(m_histograms.begin(), m_histograms.end(), [](const auto& a, const auto& b) {
        return a.baseAddress < b.baseAddress;
    });
    m_responseBufferSize = 11 * totalBinCount + 256; // up to 10 digits + comma, 256B for additional info
    m_responseBuffer = new char[m_responseBufferSize];

    addOrReplaceHandler("COUNTER", [this](vector<string> arguments) -> Result<string, string> {
        if (arguments.size() != 1 || arguments[0].empty()) {
            return { .ok = nullopt, .error = "COUNTER takes exactly one argument (0 - disabled, 1-15 - counter number)" };
        }
        int counterId;
        istringstream iss(arguments[0]);

        if (!(iss >> counterId) || !(iss.eof()) || counterId < 0 || counterId > 15) {
            return { .ok = nullopt, .error = "Counter id must be in range: 0 - disabled, 1-15 - counter number" };
        }

        return setCounterId(counterId);
    });
}

Result<string, string> TcmHistograms::resetHistograms()
{
    auto parsedResponse = processSequenceThroughHandler(
        m_handler, WinCCRequest::writeRequest(tcm_parameters::ResetHistograms, 1), false);
    if (parsedResponse.isError()) {
        return { .ok = nullopt, .error = "Failed to set reset flag:\n" + parsedResponse.getError() };
    }
    return { .ok = "Successfully reset TCM histograms", .error = nullopt };
}

Result<string, string> TcmHistograms::setCounterId(uint32_t counterId)
{
    int64_t curr = m_handler.getBoard()->at(tcm_parameters::CorrelationCountersSelect).getElectronicValueOptional().value_or(-1);
    string name = (counterId == 0 ? string("[disabled]") : to_string(counterId));
    if (curr == counterId) {
        return { .ok = "Counter " + name + " already selected", .error = nullopt };
    }

    auto parsedResponse = processSequenceThroughHandler(
        m_handler, WinCCRequest::writeRequest(tcm_parameters::CorrelationCountersSelect, counterId));

    if (parsedResponse.isError()) {
        return { .ok = nullopt, .error = "Failed to select counter " + name + ":\n" + parsedResponse.getError() };
    }
    return { .ok = "Successfully selected counter " + name, .error = nullopt };
}

bool TcmHistograms::readHistograms()
{
    if (m_histograms.size() < 2) {
        printAndPublishError("Missing histogram parameter information");
        return false;
    }

    int64_t selectedCounter = m_handler.getBoard()->at(tcm_parameters::CorrelationCountersSelect).getElectronicValueOptional().value_or(0);
    m_histograms[0].name = to_string(selectedCounter);
    bool selectableCounterEnabled = (selectedCounter != 0);

    uint32_t startAddress = selectableCounterEnabled ? m_histograms[0].baseAddress : m_histograms[1].baseAddress;
    uint32_t words = m_histograms.back().baseAddress + m_histograms.back().binCount - startAddress;

    auto res = blockRead(startAddress, true, words); // single read with unnecessary words is allegedly faster than separate reads

    if (res.isError()) {
        printAndPublishError("Failed to read TCM histograms: " + res.errors->mess);
        return false;
    } else {
        return parseHistogramData(res.content, startAddress);
    }
}

bool TcmHistograms::parseHistogramData(const vector<uint32_t>& data, uint32_t startAddress)
{
    for (auto& h : m_histograms) {
        if (h.baseAddress < startAddress) {
            h.data.clear();
            continue;
        }

        auto begin = data.begin() + (h.baseAddress - startAddress);
        auto end = begin + h.binCount;

        if (end > data.end()) {
            printAndPublishError("Incomplete histogram readout - couldn't parse all bins");
            return false;
        }

        h.data.resize(end - begin);
        copy(begin, end, h.data.begin());
    }

    return true;
}

const char* TcmHistograms::parseResponse(const string& requestResponse) const
{
    char* buffPos = m_responseBuffer;
    buffPos += sprintf(buffPos, "%" PRIu32 "\n", m_readId);
    for (const auto& h : m_histograms) {
        buffPos += sprintf(buffPos, "%s", h.name.c_str());
        for (uint32_t binVal : h.data) {
            buffPos += sprintf(buffPos, ",%" PRIu32, binVal);
        }
        *buffPos++ = '\n';
    }
    snprintf(buffPos, (m_responseBuffer + m_responseBufferSize) - buffPos, "PREV_ELAPSED,%.6fms\n%s", getPrevElapsed() * 1e-3, requestResponse.c_str()); // inserts '\0' as well

    if (static_cast<size_t>(buffPos - m_responseBuffer) > m_responseBufferSize) {
        Print::PrintWarning(name, "Response buffer overflow has occurred!");
    }
    return m_responseBuffer;
}
