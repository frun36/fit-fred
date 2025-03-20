#include "services/calibration/Calibration.h"
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include "services/calibration/utils.h"
#include "utils/utils.h"

Calibration::Calibration()
{
    // m_calibrators["TIME_ALIGN"] = make_unique<calibration::TimeAlign>;
    // m_calibrators["ADC_DELAY"] = make_unique<calibration::AdcDelay>;
    // m_calibrators["CFD_ZERO"] = make_unique<calibration::CfdZero>;
}

Calibration::Request Calibration::parseRequest(const string& req)
{
    /*
     * Request format:
     * [CALIB_NAME]
     * [CH_MASK] - 0,1,0,...,1
     * [PARAM_NAME],[PARAM_VALUE]
     * ...
     * */

    istringstream stream(req);
    string line;
    Request r;

    if (getline(stream, line)) {
        r.calibName = line;
    } else {
        throw runtime_error("Request contains no lines");
    }

    if (getline(stream, line) && line.size() == 12) {
        for (size_t i = 0; i < 12; ++i) {
            r.calibChannelMask[i] = (line[i] == '1');
        }
    } else {
        throw runtime_error("Calibration channel mask line should be 12 digits binary");
    }

    while (getline(stream, line)) {
        istringstream lineStream(line);
        string key;
        double value;

        if (std::getline(lineStream, key, ',') && (lineStream >> value)) {
            r.calibParams[key] = value;
        } else {
            throw runtime_error("Calibration parameter line should be [PARAM_NAME],[PARAM_VALUE]");
        }
    }

    return r;
}

void Calibration::processExecution()
{
    bool running;
    string req = waitForRequest(running);
    if (!running) {
        return;
    }

    Request r;
    try {
        r = parseRequest(req);
    } catch (exception& e) {
        printAndPublishError(string_utils::concatenate("Invalid request: ", e.what()));
        return;
    }

    auto calibratorIt = m_calibrators.find(r.calibName);
    if (calibratorIt == m_calibrators.end()) {
        printAndPublishError("Calibration '" + r.calibName + "' is not supported");
        return;
    }
    calibration::Calibrator& calibrator = *calibratorIt->second;

    Result<string, string> res = calibrator.run(*this, r.calibChannelMask, r.calibParams);

    if (res.isError()) {
        printAndPublishError(*res.error);
        return;
    } else {
        publishAnswer(*res.ok);
        return;
    }
}
