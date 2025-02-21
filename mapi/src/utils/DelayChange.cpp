#include "utils/DelayChange.h"
#include "board/TCM.h"
#include <unistd.h>

std::optional<DelayChange> DelayChange::fromPhysicalValues(BoardCommunicationHandler& tcm, std::optional<double> newDelayA, std::optional<double> newDelayC)
{
    if (!tcm.getBoard()->isTcm()) {
        return std::nullopt;
    }

    std::optional<int64_t> newDelayAElectronic = newDelayA.has_value() ? std::make_optional(tcm.getBoard()->calculateElectronic(string(tcm_parameters::DelayA), *newDelayA)) : std::nullopt;
    std::optional<int64_t> newDelayCElectronic = newDelayC.has_value() ? std::make_optional(tcm.getBoard()->calculateElectronic(string(tcm_parameters::DelayC), *newDelayC)) : std::nullopt;

    return fromElectronicValues(tcm, newDelayAElectronic, newDelayCElectronic);
}

std::optional<DelayChange> DelayChange::fromElectronicValues(BoardCommunicationHandler& tcm, std::optional<int64_t> newDelayA, std::optional<int64_t> newDelayC)
{
    if (!tcm.getBoard()->isTcm()) {
        return std::nullopt;
    }

    if (!newDelayA.has_value() && !newDelayC.has_value()) {
        return std::nullopt;
    }

    std::string request;
    uint32_t delayDifference = 0;

    if (newDelayA.has_value()) {
        delayDifference = abs(*newDelayA - tcm.getBoard()->at(tcm_parameters::DelayA).getElectronicValueOptional().value_or(0));
        if (delayDifference != 0) {
            WinCCRequest::appendToRequest(request, WinCCRequest::writeElectronicRequest(tcm_parameters::DelayA, *newDelayA));
        }
    }

    if (newDelayC.has_value()) {
        uint32_t cDelayDifference = abs(*newDelayC - tcm.getBoard()->at(tcm_parameters::DelayC).getElectronicValueOptional().value_or(0));
        if (cDelayDifference > delayDifference) {
            delayDifference = cDelayDifference;
        }
        if (cDelayDifference != 0) {
            WinCCRequest::appendToRequest(request, WinCCRequest::writeElectronicRequest(tcm_parameters::DelayC, *newDelayC));
        }
    }

    return delayDifference == 0 ? nullopt : make_optional<DelayChange>(request, delayDifference);
}

std::optional<DelayChange> DelayChange::fromWinCCRequest(BoardCommunicationHandler& tcm, const string& request)
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
