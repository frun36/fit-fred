#include "services/LoopingFitIndefiniteMapi.h"
#include "Board.h"
#include "BoardCommunicationHandler.h"
#include <vector>

class TcmHistograms : public LoopingFitIndefiniteMapi
{
   private:
    struct Histogram {
        string name;
        uint32_t baseAddress;
        uint32_t binCount;

        Histogram(string name, uint32_t baseAddress, uint32_t binCount)
            : name(name), baseAddress(baseAddress), binCount(binCount) {}

        bool operator<(const Histogram& o)
        {
            return baseAddress < o.baseAddress;
        }
    };

    BoardCommunicationHandler m_handler;
    vector<Histogram> m_histograms;
    bool m_doReadout = false;
    uint32_t m_readId;

    char* m_responseBuffer = nullptr;
    size_t m_responseBufferSize;

    bool handleCounterRequest(const string& request);

    bool setCounterId(uint32_t counterId);
    bool resetHistograms();

    vector<vector<uint32_t>> readHistograms();
    vector<vector<uint32_t>> parseHistogramData(const vector<uint32_t>& data, uint32_t startAddress);
    
    // Assumes data is formatted properly - needs to be ensured after read
    const char* parseResponse(const vector<vector<uint32_t>>& data, const string& requestResponse) const;

    static constexpr useconds_t ReadoutInterval = 1'000'000;
   public:
    TcmHistograms(shared_ptr<Board> tcm);

    void processExecution() override;

    ~TcmHistograms() { delete[] m_responseBuffer; }
};