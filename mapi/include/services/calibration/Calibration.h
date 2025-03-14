#pragma once

#include <array>
#include <string>
#include "services/templates/BasicFitIndefiniteMapi.h"
#include "utils/utils.h"

class Calibration : public BasicFitIndefiniteMapi
{
   private:
    Result<array<bool, 12>, string> parseRequest(bool& running);

   public:
    void processExecution() override;

    virtual Result<string, string> run(array<bool, 12> calibrationChannelMask) = 0;
};
