#pragma once

#include <string>
#include <unordered_map>
#include "services/templates/BasicFitIndefiniteMapi.h"
#include "services/calibration/Calibrator.h"

class Calibration : public BasicFitIndefiniteMapi
{
   private:
    unordered_map<string, calibration::Calibrator&> m_calibrators;

    Result<string, string> getCalibName(string req);
    Result<calibration::ChannelArray<bool>, string> getCalibChannelMask(string req);
    Result<calibration::Calibrator::Params, string> getCalibParams(string req);

   public:
    void processExecution() override;
};
