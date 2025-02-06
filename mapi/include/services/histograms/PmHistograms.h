#pragma once

#include "services/LoopingFitIndefiniteMapi.h"
#include "BoardCommunicationHandler.h"
#include "services/histograms/PmHistogramData.h"

class PmHistograms : public LoopingFitIndefiniteMapi
{
   private:
    PmHistogramData data;
    map<string, vector<const vector<uint32_t>&>> orderedHistogramData;
    BoardCommunicationHandler m_handler;

    static constexpr useconds_t ReadoutInterval = 1'000'000;
    uint32_t m_readId = 0;
    uint32_t m_fifoAddress;

    char* m_responseBuffer = nullptr;

   public:
    PmHistograms(shared_ptr<Board> pm);
    void processExecution() override;

    bool selectHistograms(const vector<string>& names);
    bool resetHistograms();
    bool switchHistogramming(bool on);

    bool readHistograms();

    const char* parseResponse(string requestResultResponse);
};
