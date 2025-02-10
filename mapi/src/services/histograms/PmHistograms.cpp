#include "services/histograms/PmHistograms.h"
#include <cstdio>
#include "PM.h"
#include "services/histograms/BinBlock.h"
#include "utils.h"

PmHistograms::PmHistograms(shared_ptr<Board> pm) : m_handler(pm)
{
    if (pm->isTcm()) {
        throw runtime_error("PmHistograms: board is a TCM");
    }

    // fetch histogram data from db - todo

    m_fifoAddress = pm->at(pm_parameters::HistogramDataReadout).baseAddress;

    addOrReplaceHandler("SELECT", [this](vector<string> arguments) -> Result<string, string> {
        return selectHistograms(arguments);
    });

    addOrReplaceHandler("HISTOGRAMMING", [this](vector<string> arguments) -> Result<string, string> {
        if (arguments.size() != 1 || (arguments[0] != "0" && arguments[1] != "1")) {
            return { .ok = nullopt, .error = "HISTOGRAMMING command takes exactly one argument: 0 or 1" };
        }
        return switchHistogramming(arguments[0] == "1");
    });

    addOrReplaceHandler("BCID_FILTER", [this](vector<string> arguments) -> Result<string, string> {
        int counterId;
        istringstream iss(arguments.size() == 1 ? arguments[0] : "");

        if (arguments[0] == "OFF") {
            return setBcIdFilter(-1);
        }

        if (!(iss >> hex >> counterId) || !iss.eof()) {
            return { .ok = nullopt, .error = "BCID_FILTER takes exactly one valid hexadecimal integer argument or the string \"OFF\"" };
        }

        return setBcIdFilter(counterId);
    });
}

Result<string, string> PmHistograms::selectHistograms(const vector<string>& names)
{
    string selected = data.selectHistograms(names);
    return { .ok = "Successfully selected histograms: " + selected, .error = nullopt };
}

Result<string, string> PmHistograms::resetHistograms()
{
    auto parsedResponse =
        processSequenceThroughHandler(m_handler, WinCCRequest::writeRequest(pm_parameters::ResetHistograms, 1), false);
    if (parsedResponse.isError()) {
        return { .ok = nullopt, .error = "Failed to set reset flag:\n" + parsedResponse.getError() };
    }
    return { .ok = "Successfully reset PM histograms", .error = nullopt };
}

Result<string, string> PmHistograms::switchHistogramming(bool on)
{
    auto parsedResponse =
        processSequenceThroughHandler(
            m_handler,
            WinCCRequest::writeRequest(pm_parameters::HistogrammingOn, static_cast<uint32_t>(on)));
    if (parsedResponse.isError()) {
        return { .ok = nullopt, .error = "Failed to set histogramming flag:\n" + parsedResponse.getError() };
    }
    return { .ok = "Successfully " + (on ? string("enabled") : string("disabled")) + " PM histogramming", .error = nullopt };
}

Result<string, string> PmHistograms::setBcIdFilter(int64_t bcId)
{
    if (bcId < 0) {
        if (m_handler.getBoard()->at(pm_parameters::BcIdFilterOn).getElectronicValue() == 0) {
            return { .ok = "BCID filter already disabled.", .error = nullopt };
        }

        auto parsedResponse = processSequenceThroughHandler(m_handler, WinCCRequest::writeElectronicRequest(pm_parameters::BcIdFilterOn, 0));
        if (parsedResponse.isError()) {
            return { .ok = nullopt, .error = "Failed to reset BCID filter enabled flag:\n" + parsedResponse.getError() };
        }
        return { .ok = "Successfully disabled BCID filter", .error = nullopt };
    }

    bool alreadyEnabled = (m_handler.getBoard()->at(pm_parameters::BcIdFilterOn).getElectronicValue() == 1);
    bool alreadySet = (m_handler.getBoard()->at(pm_parameters::BcIdToFilter).getElectronicValue() == bcId);

    if (!alreadyEnabled) {
        auto parsedResponse = processSequenceThroughHandler(m_handler, WinCCRequest::writeElectronicRequest(pm_parameters::BcIdFilterOn, 1));
        if (parsedResponse.isError()) {
            return { .ok = nullopt, .error = "Failed to set BCID filter enabled flag:\n" + parsedResponse.getError() };
        }
    }

    if (!alreadySet) {
        auto parsedResponse = processSequenceThroughHandler(m_handler, WinCCRequest::writeElectronicRequest(pm_parameters::BcIdToFilter, bcId));
        if (parsedResponse.isError()) {
            return { .ok = nullopt, .error = "Filtering is enabled, but failed to select BCID to filter:\n" + parsedResponse.getError() };
        }
    }

    return { .ok = "BCID filter " + (alreadyEnabled ? string("was already enabled") : string("successfully enabled")) + ". BCID " + to_string(bcId) + (alreadySet ? string("was already set.") : string("successfully set.")),
             .error = nullopt };
}

bool PmHistograms::readHistograms()
{
    auto operations = data.getOperations();

    for (const auto& [baseAddress, readSize] : operations) {
        auto parsedResponse =
            processSequenceThroughHandler(
                m_handler,
                WinCCRequest::writeRequest(pm_parameters::CurrentAddressInHistogramData, baseAddress),
                true);
        if (parsedResponse.isError()) {
            printAndPublishError("Error setting position in FIFO: " + parsedResponse.getError());
            return false;
        }

        auto blockReadResponse = blockRead(m_fifoAddress, false, readSize);
        if (blockReadResponse.isError()) {
            printAndPublishError("Error in histogram FIFO readout: " + blockReadResponse.errors->mess);
            return false;
        }

        if (!data.storeReadoutData(baseAddress, blockReadResponse.content)) {
            printAndPublishError("Error: invalid data readout length");
            return false;
        }
    }

    return true;
}

const char* PmHistograms::parseResponse(const string& requestResponse) const
{
    char* buffPos = m_responseBuffer;
    buffPos += sprintf(buffPos, "%08X\n", m_readId);
    for (const auto& [name, blocks] : data.getData()) {
        buffPos += sprintf(buffPos, "%s", name.c_str());
        for (const BinBlock* block : blocks) {
            if (!block->readoutEnabled) {
                continue;
            }
            if (block->isNegativeDirection) {
                for (auto it = block->data.end() - 1; it >= block->data.begin(); it--) {
                    buffPos += sprintf(buffPos, ",%X,%X", (*it) >> 16, (*it) & 0xFFFF);
                }
            } else {
                for (auto it = block->data.begin(); it < block->data.end(); it++) {
                    buffPos += sprintf(buffPos, ",%X,%X", (*it) & 0xFFFF, (*it) >> 16);
                }
            }
        }
        buffPos += sprintf(buffPos, "\n");
    }
    snprintf(buffPos, (m_responseBuffer + m_responseBufferSize) - buffPos, "%s", requestResponse.c_str()); // inserts '\0' as well, even if requestResponse is empty
    return m_responseBuffer;
}
