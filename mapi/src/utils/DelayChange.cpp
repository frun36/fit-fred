#include "utils/DelayChange.h"
#include "Alfred/print.h"
#include "board/TCM.h"
#include <unistd.h>

std::optional<DelayChange> DelayChange::fromPhysicalValues(std::shared_ptr<Board> tcm, std::optional<double> newDelayA, std::optional<double> newDelayC)
{
    if (!tcm->isTcm()) {
        return std::nullopt;
    }

    std::optional<int64_t> newDelayAElectronic = newDelayA.has_value()
                                                     ? std::make_optional(tcm->calculateElectronic(string(tcm_parameters::DelayA), *newDelayA))
                                                     : std::nullopt;
    std::optional<int64_t> newDelayCElectronic = newDelayC.has_value()
                                                     ? std::make_optional(tcm->calculateElectronic(string(tcm_parameters::DelayC), *newDelayC))
                                                     : std::nullopt;

    return fromElectronicValues(tcm, newDelayAElectronic, newDelayCElectronic);
}

uint32_t DelayChange::getDelayDifference(Board::ParameterInfo& delayParam, std::optional<int64_t> newDelay)
{
    if (!newDelay.has_value()) {
        return 0;
    }

    std::optional<int64_t> oldDelay = delayParam.getElectronicValueOptional();
    if (!oldDelay.has_value()) {
        return std::max(
            std::abs(delayParam.minValue - *newDelay),
            std::abs(delayParam.maxValue - *newDelay));
    }

    return std::abs(*oldDelay - *newDelay);
}

std::optional<DelayChange> DelayChange::fromElectronicValues(std::shared_ptr<Board> tcm, std::optional<int64_t> newDelayA, std::optional<int64_t> newDelayC)
{
    if (!tcm->isTcm()) {
        return std::nullopt;
    }

    if (!newDelayA.has_value() && !newDelayC.has_value()) {
        return std::nullopt;
    }

    std::string request;

    uint32_t delayADifference = getDelayDifference(tcm->at(tcm_parameters::DelayA), newDelayA);
    if (delayADifference != 0) {
        WinCCRequest::appendToRequest(request, WinCCRequest::writeElectronicRequest(tcm_parameters::DelayA, *newDelayA));
    }

    uint32_t delayCDifference = getDelayDifference(tcm->at(tcm_parameters::DelayC), newDelayC);
    if (delayCDifference != 0) {
        WinCCRequest::appendToRequest(request, WinCCRequest::writeElectronicRequest(tcm_parameters::DelayC, *newDelayC));
    }

    uint32_t delayDifference = std::max(delayADifference, delayCDifference);

    return delayDifference == 0 ? nullopt : make_optional<DelayChange>(request, delayDifference);
}

std::optional<DelayChange> DelayChange::fromWinCCRequest(std::shared_ptr<Board> tcm, const string& request)
{
    optional<double> newDelayA = nullopt;
    optional<double> newDelayC = nullopt;
    WinCCRequest parsedReq(request);
    for (const auto& cmd : parsedReq.getCommands()) {
        if (cmd.operation != WinCCRequest::Operation::Write) {
            throw runtime_error("Unexpected read operation");
        }

        if (cmd.name == tcm_parameters::DelayA) {
            newDelayA = cmd.value;
        } else if (cmd.name == tcm_parameters::DelayC) {
            newDelayC = cmd.value;
        } else {
            throw runtime_error("Unexpected parameter in delay change request");
        }
    }

    return DelayChange::fromPhysicalValues(tcm, newDelayA, newDelayC);
}

BoardCommunicationHandler::ParsedResponse DelayChange::apply(BasicFitIndefiniteMapi& service, BoardCommunicationHandler& tcm, bool clearReadinessChangedBits)
{
    Print::PrintWarning(service.getName(), "Applying phase/delay change: max difference " + std::to_string(this->delayDifference) + " units");
    auto parsedResponse = service.processSequenceThroughHandler(tcm, this->req);

    if (!parsedResponse.isError()) {
        // Change of delays/phase is slowed down to 1 unit/ms in the TCM logic, to avoid PLL relock.
        // For larger changes however, the relock will occur nonetheless, causing the BOARD_STATUS_SYSTEM_RESTARTED bit to be set.
        // This sleep waits for the phase shift to finish, and clears the bit
        usleep((static_cast<useconds_t>(this->delayDifference) + 10) * 1000);
        if (clearReadinessChangedBits) {
            string resetReq;
            WinCCRequest::appendToRequest(resetReq, WinCCRequest::writeRequest(tcm_parameters::SystemRestarted, 1));
            service.processSequenceThroughHandler(tcm, resetReq, false);
        }
    }

    return parsedResponse;
}
