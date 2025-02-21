#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "board/BoardCommunicationHandler.h"
#include "services/templates/BasicFitIndefiniteMapi.h"

struct DelayChange {
    const std::string req;
    const uint32_t delayDifference;

    DelayChange(const std::string& req, uint32_t delayDifference) : req(req), delayDifference(delayDifference) {}

    static std::optional<DelayChange> fromPhysicalValues(BoardCommunicationHandler& tcm, std::optional<double> newDelayA, std::optional<double> newDelayC);
    static std::optional<DelayChange> fromElectronicValues(BoardCommunicationHandler& tcm, std::optional<int64_t> newDelayA, std::optional<int64_t> newDelayC);

    static std::optional<DelayChange> fromWinCCRequest(BoardCommunicationHandler& tcm, const string& request);

    BoardCommunicationHandler::ParsedResponse apply(BasicFitIndefiniteMapi& service, BoardCommunicationHandler& tcm, bool clearReadinessChangedBits = true);
};
