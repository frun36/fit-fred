#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "BoardCommunicationHandler.h"
#include "BasicFitIndefiniteMapi.h"
#include "TCM.h"
#include "unistd.h"

struct DelayChange {
    const std::string req;
    const uint32_t delayDifference;

    DelayChange(const std::string& req, uint32_t delayDifference) : req(req), delayDifference(delayDifference) {}

    static std::optional<DelayChange> fromValues(BoardCommunicationHandler& tcm, std::optional<double> newDelayA, std::optional<double> newDelayC)
    {
        if (!tcm.getBoard()->isTcm())
            return std::nullopt;

        if (!newDelayA.has_value() && !newDelayC.has_value())
            return std::nullopt;

        std::string request;
        uint32_t delayDifference = 0;

        if (newDelayA.has_value()) {
            int64_t newDelayAElectronic = tcm.getBoard()->calculateElectronic("DELAY_A", *newDelayA);
            delayDifference = abs(newDelayAElectronic - tcm.getBoard()->at("DELAY_A").getElectronicValueOptional().value_or(0));
            if (delayDifference != 0)
                WinCCRequest::appendToRequest(request, WinCCRequest::writeRequest("DELAY_A", newDelayA.value()));
        }

        if (newDelayC.has_value()) {
            int64_t newDelayCElectronic = tcm.getBoard()->calculateElectronic("DELAY_C", *newDelayC);
            uint32_t cDelayDifference = abs(newDelayCElectronic - tcm.getBoard()->at("DELAY_C").getElectronicValueOptional().value_or(0));
            if (cDelayDifference > delayDifference)
                delayDifference = cDelayDifference;
            if (cDelayDifference != 0)
                WinCCRequest::appendToRequest(request, WinCCRequest::writeRequest("DELAY_C", newDelayC.value()));
        }

        return delayDifference == 0 ? nullopt : make_optional<DelayChange>(request, delayDifference);
    }

    static std::optional<DelayChange> fromWinCCRequest(BoardCommunicationHandler& tcm, const string& request) {
        optional<double> newDelayA = nullopt;
        optional<double> newDelayC = nullopt;
        for (auto& cmd : WinCCRequest(request).getCommands()) {
            if (cmd.operation != WinCCRequest::Operation::Write)
                throw runtime_error("Unexpected read operation");

            if (cmd.name == tcm_parameters::DelayA)
                newDelayA = cmd.value;
            else if (cmd.name == tcm_parameters::DelayC)
                newDelayC = cmd.value;
            else
                throw runtime_error("Unexpected parameter in delay change request");
        }

        return DelayChange::fromValues(tcm, newDelayA, newDelayC);
    }

    BoardCommunicationHandler::ParsedResponse apply(BasicFitIndefiniteMapi& service, BoardCommunicationHandler& tcm)
    {
        auto parsedResponse = service.processSequenceThroughHandler(tcm, this->req);

        if (!parsedResponse.isError()) {
            // Change of delays/phase is slowed down to 1 unit/ms in the TCM logic, to avoid PLL relock.
            // For larger changes however, the relock will occur nonetheless, causing the BOARD_STATUS_SYSTEM_RESTARTED bit to be set.
            // This sleep waits for the phase shift to finish, and clears the bit
            usleep((static_cast<useconds_t>(this->delayDifference) + 10) * 1000);
            string resetReq;
            WinCCRequest::appendToRequest(resetReq, WinCCRequest::writeRequest("BOARD_STATUS_SYSTEM_RESTARTED", 1));
            service.processSequenceThroughHandler(tcm, resetReq, false);
        }

        return parsedResponse;
    }
};