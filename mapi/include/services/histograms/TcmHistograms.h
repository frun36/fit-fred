#include "services/LoopingFitIndefiniteMapi.h"
#include "Board.h"
#include "BoardCommunicationHandler.h"
#include <set>

class TcmHistograms : public LoopingFitIndefiniteMapi
{
   private:
    struct Histogram {
        const string name;
        const uint32_t relativeAddress;
        const uint32_t binCount;

        Histogram(string name, uint32_t relativeAddress, uint32_t binCount)
            : name(name), relativeAddress(relativeAddress), binCount(binCount) {}

        bool operator<(const Histogram& o)
        {
            return relativeAddress < o.relativeAddress;
        }
    };

    BoardCommunicationHandler m_handler;
    Board::ParameterInfo m_baseParam;
    set<Histogram> m_histograms;
    bool m_doReadout = false;
    uint32_t m_readId;

    static constexpr uint32_t ResponseBufferSize = 16384;
    char m_responseBuffer[ResponseBufferSize] = {};

    bool handleCounterRequest(const string& request);

    bool setCounterId(uint32_t counterId);
    bool resetHistograms();

    vector<vector<uint32_t>> readHistograms();
    string parseResponse(const vector<vector<uint32_t>>&& data) const;

    static constexpr useconds_t ReadoutInterval = 1'000'000;
   public:
    TcmHistograms(shared_ptr<Board> tcm);

    void processExecution() override;
};