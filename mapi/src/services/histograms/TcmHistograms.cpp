#include "services/histograms/TcmHistograms.h"
#include "TCM.h"
#include "utils.h"
#include <cctype>
#include <cstdio>
#include <sstream>
#include <string>

vector<TcmHistograms::Histogram> TcmHistograms::fetchHistogramInfo(shared_ptr<Board> tcm)
{
    if (!tcm->isTcm()) {
        throw runtime_error("TcmHistograms: board is not a TCM");
    }

    vector<Histogram> histograms;
    for (auto name : tcm_parameters::getAllHistograms()) {
        const Board::ParameterInfo& param = tcm->at(name);
        string paramName = string(name);
        size_t lastUnderscorePos = paramName.find_last_of('_');
        string counterName = (lastUnderscorePos == string::npos ? paramName : paramName.substr(lastUnderscorePos + 1));
        histograms.emplace_back(counterName, param.baseAddress, param.regBlockSize);
    }

    sort(histograms.begin(), histograms.end(), [](const auto& a, const auto& b) {
        return a.baseAddress < b.baseAddress;
    });

    return histograms;
}

TcmHistograms::TcmHistograms(shared_ptr<Board> tcm) : m_handler(tcm), m_histograms(fetchHistogramInfo(tcm))
{
    if (!tcm->isTcm()) {
        throw runtime_error("TcmHistograms: board is not a TCM");
    }

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

Result<string, string> TcmHistograms::readAndStoreHistograms()
{
    if (m_histograms.size() < 2) {
        return { .ok = nullopt, .error = "Missing histogram parameter information" };
    }

    int64_t selectedCounter = m_handler.getBoard()->at(tcm_parameters::CorrelationCountersSelect).getElectronicValueOptional().value_or(0);
    bool selectableCounterEnabled = (selectedCounter != 0);

    uint32_t startAddress = selectableCounterEnabled ? m_histograms[0].baseAddress : m_histograms[1].baseAddress;
    uint32_t words = m_histograms.back().baseAddress + m_histograms.back().binCount - startAddress;

    auto res = blockRead(startAddress, true, words); // single read with unnecessary words is allegedly faster than separate reads
    if (res.isError()) {
        return { .ok = nullopt, .error = "Failed to read TCM histograms: " + res.errors->mess };
    } else {
        return parseHistogramData(res.content, startAddress)
                   ? Result<string, string>{ .ok = "", .error = nullopt }
                   : Result<string, string>{ .ok = nullopt, .error = "Incomplete histogram readout - couldn't parse all bins" };
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
            return false;
        }

        h.data.resize(end - begin);
        copy(begin, end, h.data.begin());
    }

    return true;
}

void TcmHistograms::parseResponse(ostringstream& oss) const
{
    for (const auto& h : m_histograms) {
        oss << h.name.c_str();
        for (uint32_t binVal : h.data) {
            oss << "," << binVal;
        }
        oss << "\n";
    }
}
