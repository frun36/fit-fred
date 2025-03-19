#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "board/BoardCommunicationHandler.h"
#include "services/templates/BasicFitIndefiniteMapi.h"

class DelayChange
{
   private:
    static uint32_t getDelayDifference(Board::ParameterInfo& delayParam, std::optional<int64_t> newDelay);

   public:
    const std::string req;
    const uint32_t delayDifference;

    DelayChange(const std::string& req, uint32_t delayDifference) : req(req), delayDifference(delayDifference) {}

    static std::optional<DelayChange> fromPhysicalValues(std::shared_ptr<Board> tcm, std::optional<double> newDelayA, std::optional<double> newDelayC);
    static std::optional<DelayChange> fromElectronicValues(std::shared_ptr<Board> tcm, std::optional<int64_t> newDelayA, std::optional<int64_t> newDelayC);

    static std::optional<DelayChange> fromWinCCRequest(std::shared_ptr<Board> tcm, const string& request);

    BoardCommunicationHandler::ParsedResponse apply(BasicFitIndefiniteMapi& service, BoardCommunicationHandler& tcm, bool clearReadinessChangedBits = true);
};
