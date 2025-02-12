#pragma once

#include "Board.h"
#include "BoardCommunicationHandler.h"
#include "services/histograms/BoardHistograms.h"
#include <vector>

class TcmHistograms : public BoardHistograms
{
   private:
    struct Histogram {
        string name;
        uint32_t baseAddress;
        uint32_t binCount;
        vector<uint32_t> data;

        Histogram(string name, uint32_t baseAddress, uint32_t binCount)
            : name(name), baseAddress(baseAddress), binCount(binCount)
        {
            data.reserve(binCount);
        }
    };

    BoardCommunicationHandler m_handler;
    vector<Histogram> m_histograms; // Needs to be sorted by base address!

    size_t m_responseBufferSize = 0;
    char* m_responseBuffer = nullptr;

    Result<string, string> setCounterId(uint32_t counterId);
    Result<string, string> resetHistograms() override;

    bool readHistograms() override;
    bool parseHistogramData(const vector<uint32_t>& data, uint32_t startAddress);

    // Assumes data is formatted properly - needs to be ensured after read
    const char* parseResponse(const string& requestResponse) const override;

   public:
    TcmHistograms(shared_ptr<Board> tcm);

    ~TcmHistograms() { delete[] m_responseBuffer; }
};
