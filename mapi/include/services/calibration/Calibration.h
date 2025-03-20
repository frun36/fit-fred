#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include "services/templates/BasicFitIndefiniteMapi.h"
#include "services/calibration/Calibrator.h"

class Calibration : public BasicFitIndefiniteMapi
{
   private:
    unordered_map<string, unique_ptr<calibration::Calibrator>> m_calibrators;

    struct Request {
        string calibName;
        calibration::ChannelArray<bool> calibChannelMask;
        unordered_map<string, double> calibParams;
    };
    static Request parseRequest(const string& req);

   public:
    Calibration();
    void processExecution() override;
};
