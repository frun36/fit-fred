#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "BoardCommunicationHandler.h"
#include "services/BasicFitIndefiniteMapi.h"
#include "TCM.h"
#include "unistd.h"

struct DelayChange {
    const std::string req;
    const uint32_t delayDifference;

    DelayChange(const std::string& req, uint32_t delayDifference) : req(req), delayDifference(delayDifference) {}

    static std::optional<DelayChange> fromValues(BoardCommunicationHandler& tcm, std::optional<double> newDelayA, std::optional<double> newDelayC);

    static std::optional<DelayChange> fromWinCCRequest(BoardCommunicationHandler& tcm, const string& request);

    BoardCommunicationHandler::ParsedResponse apply(BasicFitIndefiniteMapi& service, BoardCommunicationHandler& tcm, bool clearReadinessChangedBits = true);
};