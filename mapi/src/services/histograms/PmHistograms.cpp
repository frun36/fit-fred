#include "services/histograms/PmHistograms.h"
#include "PM.h"
#include "utils.h"
#include "PmHistograms.h"

PmHistograms::PmHistograms(shared_ptr<Board> pm) : m_handler(pm)
{
    if (pm->isTcm()) {
        throw runtime_error("PmHistograms: board is a TCM");
    }

    size_t totalBinCount = 0;
    // fetch histogram data from db

    m_fifoAddress = pm->at(pm_parameters::HistogramDataReadout).baseAddress;

    addOrReplaceHandler("SELECT", [this](string request) {
        auto splitResult = string_utils::Splitter::getAll(request, ',');
        if (splitResult.isError()) {
            return false;
        }

        vector<string>& names = splitResult.ok.value();
        names.erase(names.begin());
        return selectHistograms(names);
    });

    addOrReplaceHandler("RESET", [this](string) {
        return resetHistograms();
    });

    addOrReplaceHandler("HISTOGRAMMING", [this](string request) {
        auto splitResult = string_utils::Splitter::getAll(request, ',');
        if (splitResult.isError()) {
            return false;
        }

        vector<string>& params = splitResult.ok.value();
        if (params.size() != 2) {
            return false;
        }

        return switchHistogramming(params[1] == "1");
    });
}

void PmHistograms::processExecution()
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

    if (!readHistograms()) {
        return;
    }

    string requestResultString = (requestResult.isEmpty() || requestResult.isError) ? string("") : requestResult;
    publishAnswer(parseResponse(requestResultString));
    m_readId++;
}

bool PmHistograms::selectHistograms(const vector<string>& names)
{
    data.selectHistograms(names);
    return true;
}

bool PmHistograms::resetHistograms()
{
    auto parsedResponse = processSequenceThroughHandler(m_handler, WinCCRequest::writeRequest(pm_parameters::ResetHistograms, 1), false);
    return !parsedResponse.isError();
}

bool PmHistograms::switchHistogramming(bool on)
{
    auto parsedResponse = processSequenceThroughHandler(m_handler, WinCCRequest::writeRequest(pm_parameters::HistogrammingOn, static_cast<uint32_t>(on)));
    return !parsedResponse.isError();
}

bool PmHistograms::readHistograms()
{
    auto operations = data.getOperations();

    for (const auto& [baseAddress, readSize] : operations) {
        auto parsedResponse = processSequenceThroughHandler(m_handler, WinCCRequest::writeRequest(pm_parameters::CurrentAddressInHistogramData, baseAddress), true);
        if (parsedResponse.isError()) {
            printAndPublishError("Error setting position in FIFO: " + parsedResponse.getError());
            return false;
        }
        
        auto blockReadResponse = blockRead(m_fifoAddress, false, readSize);
        if (blockReadResponse.isError()) {
            printAndPublishError("Error in histogram FIFO readout: " + blockReadResponse.errors->mess);
            return false;
        }

        if(!data.storeReadoutData(baseAddress, blockReadResponse.content)) {
            printAndPublishError("Error: invalid data readout length");
            return false;
        }
    }

    return true;
}
