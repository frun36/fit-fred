#include "services/histograms/PmHistogramController.h"

PmHistogramController::PmHistogramController(shared_ptr<Board> pm, std::unordered_map<std::string, FitData::PmHistogram> histograms)
    : m_data(pm, histograms),
      m_handler(pm),
      m_fifoAddress(pm->isTcm() ? throw runtime_error("PmHistogramController: board is a TCM")
                                : pm->at(pm_parameters::HistogramReadout).baseAddress)
{
}

Result<string, string> PmHistogramController::selectHistograms(const vector<string>& names)
{
    string selected = m_data.selectHistograms(names);
    return { .ok = "Successfully selected histograms: " + selected, .error = nullopt };
}

Result<string, string> PmHistogramController::resetHistograms(BasicFitIndefiniteMapi& mapi)
{
    auto parsedResponse =
        mapi.processSequenceThroughHandler(m_handler, WinCCRequest::writeRequest(pm_parameters::ResetHistograms, 1), false);
    if (parsedResponse.isError()) {
        return { .ok = nullopt, .error = "Failed to set reset flag:\n" + parsedResponse.getError() };
    }
    return { .ok = "Successfully reset PM histograms", .error = nullopt };
}

Result<string, string> PmHistogramController::switchHistogramming(BasicFitIndefiniteMapi& mapi, bool on)
{
    auto parsedResponse =
        mapi.processSequenceThroughHandler(
            m_handler,
            WinCCRequest::writeRequest(pm_parameters::HistogrammingOn, static_cast<uint32_t>(on)));
    if (parsedResponse.isError()) {
        return { .ok = nullopt, .error = "Failed to write histogramming flag:\n" + parsedResponse.getError() };
    }
    return { .ok = "Successfully " + (on ? string("enabled") : string("disabled")) + " PM histogramming", .error = nullopt };
}

Result<string, string> PmHistogramController::setBcIdFilter(BasicFitIndefiniteMapi& mapi, int64_t bcId)
{
    if (bcId < 0) {
        if (m_handler.getBoard()->at(pm_parameters::BcIdFilterOn).getElectronicValueOptional().value_or(-1) == 0) {
            return { .ok = "BCID filter already disabled.", .error = nullopt };
        }

        auto parsedResponse =
            mapi.processSequenceThroughHandler(m_handler, WinCCRequest::writeElectronicRequest(pm_parameters::BcIdFilterOn, 0));
        if (parsedResponse.isError()) {
            return { .ok = nullopt, .error = "Failed to reset BCID filter enabled flag:\n" + parsedResponse.getError() };
        }
        return { .ok = "Successfully disabled BCID filter", .error = nullopt };
    }

    bool alreadyEnabled = (m_handler.getBoard()->at(pm_parameters::BcIdFilterOn).getElectronicValueOptional().value_or(-1) == 1);
    bool alreadySet = (m_handler.getBoard()->at(pm_parameters::BcIdToFilter).getElectronicValueOptional().value_or(-1) == bcId);

    if (!alreadyEnabled) {
        auto parsedResponse =
            mapi.processSequenceThroughHandler(m_handler, WinCCRequest::writeElectronicRequest(pm_parameters::BcIdFilterOn, 1));
        if (parsedResponse.isError()) {
            return { .ok = nullopt, .error = "Failed to set BCID filter enabled flag:\n" + parsedResponse.getError() };
        }
    }

    if (!alreadySet) {
        auto parsedResponse = mapi.processSequenceThroughHandler(m_handler, WinCCRequest::writeElectronicRequest(pm_parameters::BcIdToFilter, bcId));
        if (parsedResponse.isError()) {
            return {
                .ok = nullopt,
                .error = "Filtering is enabled, but selecting BCID " + to_string(bcId) + " to filter has failed:\n" + parsedResponse.getError()
            };
        }
    }

    return { .ok = "BCID filter " + (alreadyEnabled ? string("was already enabled") : string("successfully enabled")) +
                   ". BCID " + to_string(bcId) + (alreadySet ? string(" was already set.") : string(" successfully set.")),
             .error = nullopt };
}

Result<string, string> PmHistogramController::readAndStoreHistograms(BasicFitIndefiniteMapi& mapi)
{
    Print::PrintData("readHistograms start");
    auto operations = m_data.getOperations();

    for (const auto& [baseAddress, readSize] : operations) {
        Print::PrintData("Setting address " + to_string(baseAddress) + " size " + to_string(readSize));
        auto parsedResponse =
            mapi.processSequenceThroughHandler(
                m_handler,
                WinCCRequest::writeRequest(pm_parameters::CurrentAddressInHistogramData, baseAddress),
                true);
        if (parsedResponse.isError()) {
            return { .ok = nullopt, .error = "Error setting position in FIFO: " + parsedResponse.getError() };
        }

        Print::PrintData("Performing FIFO readout");
        auto blockReadResponse = mapi.blockRead(m_fifoAddress, false, readSize);
        if (blockReadResponse.isError()) {
            return { .ok = nullopt, .error = "Error in histogram FIFO readout: " + blockReadResponse.errors->mess };
        }

        if (!m_data.storeReadoutData(baseAddress, blockReadResponse.content)) {
            return { .ok = nullopt, .error = "Error: invalid data readout length" };
        }
        Print::PrintData("Data read out and stored");
    }

    Print::PrintData("readHistograms end");
    return { .ok = "", .error = nullopt };
}
