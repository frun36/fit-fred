#include "services/BasicFitIndefiniteMapi.h"
#include "Board.h"
#include "BoardCommunicationHandler.h"
#include <set>

class TcmHistograms : public BasicFitIndefiniteMapi
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

    void handleRequest(const string& request);

    bool setCounterId(uint32_t counterId);
    bool startHistogramming();
    bool stopHistogramming();
    bool resetHistograms();

    vector<vector<uint32_t>> readHistograms();
    string parseResponse(vector<vector<uint32_t>> data) const;
   public:
    TcmHistograms(shared_ptr<Board> tcm);

    void processExecution() override;
};