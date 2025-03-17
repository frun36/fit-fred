#pragma once

#include <sys/types.h>
#include <array>
#include <cstdint>
#include "services/histograms/PmHistogramData.h"

namespace calibration
{

template <typename T>
using ChannelArray = std::array<T, 12>;

useconds_t getSleepTime(uint32_t expectedEntries, double refRateHz);

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

    ChannelHistogramInfo(Status status, uint32_t sampleCount = 0, double mean = 0., double stddev = 0.)
        : status(status), sampleCount(sampleCount), mean(mean), stddev(stddev) {}
};

ChannelHistogramInfo processChannelTimeHistogram(const BlockView& data, uint32_t chIdx, uint32_t expectedEntries);
ChannelHistogramInfo processChannelAmplitudeHistogram(const BlockView& data, uint32_t chIdx, uint32_t expectedEntries);
ChannelArray<ChannelHistogramInfo> processTimeHistograms(const BlockView& data, uint32_t expectedEntries);
ChannelArray<ChannelHistogramInfo> processAmplitudeHistograms(const BlockView& data, uint32_t expectedEntries);

} // namespace calibration
