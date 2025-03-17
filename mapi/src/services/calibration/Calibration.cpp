#include <algorithm>
#include <exception>
#include <iomanip>
#include <sstream>
#include "services/histograms/BinBlock.h"
#include "services/calibration/Calibration.h"

useconds_t Calibration::getSleepTime(uint32_t expectedEntries)
{
    static constexpr useconds_t MinSleep = 200'000;
    useconds_t expectedSleep = 1e6 * static_cast<double>(expectedEntries) / m_refRateHz;
    return max(MinSleep, expectedSleep);
}

Result<ChannelArray<bool>, string> Calibration::parseRequest(bool& running)
{
    ChannelArray<bool> calibrationChannelMask = { false };

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
        return sqrt(static_cast<double>(sampleSumOfSquares) / static_cast<double>(sampleCount) - mean * mean);
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

Calibration::ChannelHistogramInfo Calibration::processChannelAmplitudeHistogram(const BlockView& data, uint32_t chIdx, uint32_t expectedEntries)
{
    if (chIdx >= 12) {
        return ChannelHistogramInfo::badChannelIdx();
    }

    ostringstream nameAdc0;
    nameAdc0 << "CH" << setw(2) << chIdx << "ADC0";
    const vector<const BinBlock*>& histAdc0 = data.at(nameAdc0.str());

    ostringstream nameAdc1;
    nameAdc1 << "CH" << setw(2) << chIdx << "ADC1";
    const vector<const BinBlock*>& histAdc1 = data.at(nameAdc1.str());

    if (histAdc0.size() != histAdc1.size() || histAdc0.size() < 1) {
        return ChannelHistogramInfo::badHistogramInfo();
    }

    int32_t a0 = histAdc0[0]->isNegativeDirection ? histAdc0[0]->startBin - 2 * histAdc0[0]->data.size() + 1
                                                  : histAdc0[0]->startBin;
    BinTracker t(a0);

    for (auto itAdc0Block = histAdc0.begin(), itAdc1Block = histAdc1.begin();
         itAdc0Block < histAdc0.end() && itAdc1Block < histAdc1.end();
         itAdc0Block++, itAdc1Block++) {
        // Check homogeneity of ADC0 and ADC1 blocks - ToDo: move elsewhere?
        if ((*itAdc0Block)->data.size() != (*itAdc1Block)->data.size() ||
            (*itAdc0Block)->isNegativeDirection != (*itAdc1Block)->isNegativeDirection ||
            (*itAdc0Block)->startBin != (*itAdc1Block)->startBin) {
            return ChannelHistogramInfo::badHistogramInfo();
        }

        if ((*itAdc0Block)->isNegativeDirection) {
            for (auto itAdc0 = (*itAdc0Block)->data.rbegin(), itAdc1 = (*itAdc1Block)->data.rbegin();
                 itAdc0 < (*itAdc0Block)->data.rend() && itAdc1 < (*itAdc1Block)->data.rend();
                 itAdc0++, itAdc1++) {
                t.addBin((*itAdc0 >> 16) + (*itAdc1 >> 16));
                t.addBin((*itAdc0 & 0xFFFF) + (*itAdc1 & 0xFFFF));
            }
        } else {
            for (auto itAdc0 = (*itAdc0Block)->data.begin(), itAdc1 = (*itAdc1Block)->data.begin();
                 itAdc0 < (*itAdc0Block)->data.end() && itAdc1 < (*itAdc1Block)->data.end();
                 itAdc0++, itAdc1++) {
                t.addBin((*itAdc0 & 0xFFFF) + (*itAdc1 & 0xFFFF));
                t.addBin((*itAdc0 >> 16) + (*itAdc1 >> 16));
            }
        }
    }

    if (t.getSampleCount() < 0.8 * expectedEntries) {
        return ChannelHistogramInfo::notEnoughEntries(t.getSampleCount());
    }

    return ChannelHistogramInfo::ok(t.getSampleCount(), t.getMean(), t.getStddev());
}

ChannelArray<Calibration::ChannelHistogramInfo> Calibration::processTimeHistograms(const BlockView& data, uint32_t expectedEntries)
{
    ChannelArray<ChannelHistogramInfo> res = {
        processChannelTimeHistogram(data, 0, expectedEntries),
        processChannelTimeHistogram(data, 1, expectedEntries),
        processChannelTimeHistogram(data, 2, expectedEntries),
        processChannelTimeHistogram(data, 3, expectedEntries),
        processChannelTimeHistogram(data, 4, expectedEntries),
        processChannelTimeHistogram(data, 5, expectedEntries),
        processChannelTimeHistogram(data, 6, expectedEntries),
        processChannelTimeHistogram(data, 7, expectedEntries),
        processChannelTimeHistogram(data, 8, expectedEntries),
        processChannelTimeHistogram(data, 9, expectedEntries),
        processChannelTimeHistogram(data, 10, expectedEntries),
        processChannelTimeHistogram(data, 11, expectedEntries),
    };
    return res;
}

ChannelArray<Calibration::ChannelHistogramInfo> Calibration::processAmplitudeHistograms(const BlockView& data, uint32_t expectedEntries)
{
    ChannelArray<ChannelHistogramInfo> res = {
        processChannelAmplitudeHistogram(data, 0, expectedEntries),
        processChannelAmplitudeHistogram(data, 1, expectedEntries),
        processChannelAmplitudeHistogram(data, 2, expectedEntries),
        processChannelAmplitudeHistogram(data, 3, expectedEntries),
        processChannelAmplitudeHistogram(data, 4, expectedEntries),
        processChannelAmplitudeHistogram(data, 5, expectedEntries),
        processChannelAmplitudeHistogram(data, 6, expectedEntries),
        processChannelAmplitudeHistogram(data, 7, expectedEntries),
        processChannelAmplitudeHistogram(data, 8, expectedEntries),
        processChannelAmplitudeHistogram(data, 9, expectedEntries),
        processChannelAmplitudeHistogram(data, 10, expectedEntries),
        processChannelAmplitudeHistogram(data, 11, expectedEntries),
    };
    return res;
}
