#include "services/calibration/Calibration.h"

void Calibration::processExecution()
{
    bool running;
    string req = waitForRequest(running);
    if (!running) {
        return;
    }
    /*
     * Request format:
     * [CALIB_NAME]
     * [CH_MASK] - 0,1,0,...,1
     * [PARAM_NAME],[PARAM_VALUE]
     * ...
     * */

    Result<string, string> calibName = getCalibName(req);
    if (calibName.isError()) {
        printAndPublishError("Invalid request: " + *calibName.error);
        return;
    }
    auto calibratorIt = m_calibrators.find(*calibName.ok);
    if (calibratorIt == m_calibrators.end()) {
        printAndPublishError("Calibration '" + *calibName.ok + "' is not supported");
        return;
    }
    calibration::Calibrator& calibrator = calibratorIt->second;

    Result<calibration::ChannelArray<bool>, string> calibChannelMask = getCalibChannelMask(req);
    if (calibChannelMask.isError()) {
        printAndPublishError("Invalid request: " + *calibName.error);
        return;
    }

    Result<calibration::Calibrator::Params, string> calibParams = getCalibParams(req);
    if (calibParams.isError()) {
        printAndPublishError("Invalid request: " + *calibName.error);
        return;
    }

    Result<string, string> res = calibrator.run(*this, *calibChannelMask.ok, *calibParams.ok);

    if (res.isError()) {
        printAndPublishError(*res.error);
    } else {
        publishAnswer(*res.ok);
    }
}
