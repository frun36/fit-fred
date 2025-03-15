#include <exception>
#include <iomanip>
#include <sstream>
#include "services/histograms/BinBlock.h"
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
}

class BinTracker
{
   private:
    int32_t startBin;
    uint32_t sampleCount = 0;
    int64_t sampleSum = 0;
    uint64_t sampleSumOfSquares = 0;
    uint32_t currBinIdx = 0;
    uint32_t currTime = 0;

   public:
    BinTracker(int32_t startBin) : startBin(startBin) {}

    uint32_t getSampleCount() const
    {
        return sampleCount;
    }

    double getMean() const
    {
        return static_cast<double>(sampleSum) / static_cast<double>(sampleCount);
    }

    double getStddev() const
    {
        double mean = static_cast<double>(sampleSum) / static_cast<double>(sampleCount);
        return static_cast<double>(sampleSumOfSquares) / static_cast<double>(sampleCount) - mean * mean;
    }

    void addBin(uint16_t val)
    {
        sampleCount += val;
        currTime = startBin * currBinIdx;
        sampleSum += val * currTime;
        sampleSumOfSquares += val * currTime * currTime;
        currBinIdx++;
    }
};

Calibration::ChannelHistogramInfo Calibration::processChannelTimeHistogram(const BlockView& data, uint32_t chIdx, uint32_t expectedEntries)
{
    if (chIdx >= 12) {
        return ChannelHistogramInfo::badChannelIdx();
    }

    ostringstream histName;
    histName << "CH" << setw(2) << chIdx << "TIME";
    const vector<const BinBlock*>& hist = data.at(histName.str());

    if (hist.size() < 1) {
        return ChannelHistogramInfo::badHistogramInfo();
    }

    int32_t t0 = hist[0]->isNegativeDirection ? hist[0]->startBin - 2 * hist[0]->data.size() + 1
                                              : hist[0]->startBin;
    BinTracker t(t0);
    for (auto binBlock : hist) {
        if (binBlock->isNegativeDirection) {
            for (auto it = binBlock->data.rbegin(); it < binBlock->data.rend(); it++) {
                t.addBin(*it >> 16);
                t.addBin(*it & 0xFFFF);
            }
        } else {
            for (auto it = binBlock->data.begin(); it < binBlock->data.end(); it++) {
                t.addBin(*it & 0xFFFF);
                t.addBin(*it >> 16);
            }
        }
    }

    if (t.getSampleCount() < 0.8 * expectedEntries) {
        return ChannelHistogramInfo::notEnoughEntries(t.getSampleCount());
    }

    return ChannelHistogramInfo::ok(t.getSampleCount(), t.getMean(), t.getStddev());
}
