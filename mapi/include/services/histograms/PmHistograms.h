#pragma once

#include "BoardCommunicationHandler.h"
#include "services/histograms/BoardHistograms.h"
#include "services/histograms/PmHistogramData.h"

class PmHistograms : public BoardHistograms
{
   private:
    PmHistogramData data;
    BoardCommunicationHandler m_handler;

    static constexpr useconds_t ReadoutInterval = 1'000'000;
    uint32_t m_readId = 0;
    uint32_t m_fifoAddress;

    size_t m_responseBufferSize = 0;
    char* m_responseBuffer = nullptr;

   public:
    PmHistograms(shared_ptr<Board> pm, std::unordered_map<std::string, FitData::PmHistogram> histograms);

    Result<string, string> selectHistograms(const vector<string>& names);
    Result<string, string> resetHistograms() override;
    Result<string, string> switchHistogramming(bool on);
    Result<string, string> setBcIdFilter(int64_t bcId);

    bool readHistograms() override;

    const char* parseResponse(const string& requestResponse) const override;

    ~PmHistograms() { delete[] m_responseBuffer; }
};
