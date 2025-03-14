#include <exception>
#include "services/calibration/Calibration.h"

Result<array<bool, 12>, string> Calibration::parseRequest(bool& running)
{
    array<bool, 12> calibrationChannelMask = { false };

    string request = waitForRequest(running);
    auto splitResult = string_utils::Splitter::getAll(request, ',');
    if (splitResult.isError()) {
        return {
            .ok = nullopt,
            .error = "Expected a comma separated list of integers from range [1, 12] representing the channels to calibrate"
        };
    }
    vector<string> channels = *splitResult.ok;

    for (const auto& ch : channels) {
        try {
            int chInt = std::stoi(ch);
            if (chInt < 1 || chInt > 12) {
                return { .ok = nullopt, .error = "Channel number " + ch + " is not withing valid range [1, 12]" };
            }
            calibrationChannelMask[chInt - 1] = true;
        } catch (std::exception& e) {
            return { .ok = nullopt, .error = "Invalid channel number: " + ch };
        }
    }

    return { .ok = calibrationChannelMask, .error = nullopt };
}

void Calibration::processExecution()
{
    bool running;
    const auto calibrationChannelMask = parseRequest(running);
    if (!running) {
        return;
    }

    if (calibrationChannelMask.isError()) {
        printAndPublishError(*calibrationChannelMask.error);
        return;
    }

    const auto& result = run(*calibrationChannelMask.ok);
    if (result.isOk()) {
        publishAnswer(*result.ok);
    } else {
        printAndPublishError(*result.error);
    }
}
