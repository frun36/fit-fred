#pragma once

#include <array>
#include <string>
#include "services/templates/BasicFitIndefiniteMapi.h"
#include "services/histograms/PmHistogramData.h"
#include "utils/utils.h"

template <typename T>
using ChannelArray = array<T, 12>;

class Calibration : public BasicFitIndefiniteMapi
{
   protected:
    struct ChannelHistogramInfo {
        enum class Status {
            Ok,
            NotEnoughEntries,
            BadChannelIdx,
            BadHistogramInfo,
        };

        const Status status;
        const uint32_t sampleCount;
        const double mean;
        const double stddev;

        static ChannelHistogramInfo ok(uint32_t sampleCount, double mean, double stddev)
        {
            return ChannelHistogramInfo(Status::Ok, sampleCount, mean, stddev);
        }

        static ChannelHistogramInfo notEnoughEntries(uint32_t sampleCount)
        {
            return ChannelHistogramInfo(Status::NotEnoughEntries, sampleCount);
        }

        static ChannelHistogramInfo badChannelIdx()
        {
            return ChannelHistogramInfo(Status::BadChannelIdx);
        }

        static ChannelHistogramInfo badHistogramInfo()
        {
            return ChannelHistogramInfo(Status::BadHistogramInfo);
        }

       private:
        ChannelHistogramInfo(Status status, uint32_t sampleCount = 0, double mean = 0., double stddev = 0.)
            : status(status), sampleCount(sampleCount), mean(mean), stddev(stddev) {}
    };

   private:
    const double m_refRateHz = 1000.;

    ChannelHistogramInfo processChannelTimeHistogram(const BlockView& data, uint32_t chIdx, uint32_t expectedEntries);
    ChannelHistogramInfo processChannelAmplitudeHistogram(const BlockView& data, uint32_t chIdx, uint32_t expectedEntries);

    Result<ChannelArray<bool>, string> parseRequest(bool& running);

   public:
    void processExecution() override;

    virtual Result<string, string> run(ChannelArray<bool> calibrationChannelMask) = 0;

    useconds_t getSleepTime(uint32_t expectedEntries);

    ChannelArray<ChannelHistogramInfo> processTimeHistograms(const BlockView& data);
    ChannelArray<ChannelHistogramInfo> processAmplitudeHistograms(const BlockView& data);
};
