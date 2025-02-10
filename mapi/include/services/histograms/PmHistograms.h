#pragma once

#include "services/LoopingFitIndefiniteMapi.h"
#include "BoardCommunicationHandler.h"
#include "services/histograms/PmHistogramData.h"

class PmHistograms : public LoopingFitIndefiniteMapi
{
   private:
    PmHistogramData data;
    BoardCommunicationHandler m_handler;

    static constexpr useconds_t ReadoutInterval = 1'000'000;
    uint32_t m_readId = 0;
    uint32_t m_fifoAddress;

    char* m_responseBuffer = nullptr;
    size_t m_responseBufferSize = 0;

   public:
    PmHistograms(shared_ptr<Board> pm);
    void processExecution() override;

    bool selectHistograms(const vector<string>& names);
    bool resetHistograms();
    bool switchHistogramming(bool on);

    bool readHistograms();

    const char* parseResponse(string requestResultResponse);
};
